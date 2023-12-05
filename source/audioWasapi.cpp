#include "include.hpp"
#include "audioWasapi.hpp"

START_SCOPE(audioWasapi)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, uint frameCount, HWND clipAreaWindow)
{
	State* state = {};
	allocateMemory(sizeof(State), (void**)&state);
    state->frameCount = frameCount;
    state->inputLoaderCount = 0;
    state->clipAreaWindow = clipAreaWindow;
    
    createEvent(&state->audioCallback);
    createEvent(&state->endpointLoaderStartEvent);
    createEvent(&state->endpointLoaderFinishEvent);
    createEvent(&state->processorStartEvent);
    createEvent(&state->processorFinishEvent);

    createDynamicArray(&state->inputTrackArrayHandle, sizeof(HWND));
    createDynamicArray(&state->startEventArrayHandle, sizeof(HANDLE));

    createSemaphore(&state->exitSemaphore);
    createSemaphore(&state->finishSemaphore);

	createWindowClass(L"wasapiWindowClass", windowCallback);
	createChildWindow(L"wasapiWindowClass", parent, state, window);

    SendMessage(*window, WM_CREATECHILD, 0, 0);
}
ushort getAudioEngineSubFormat(WAVEFORMATEXTENSIBLE* audioEngineFormat)
{
	ushort format = 0;
	if (IsEqualGUID(audioEngineFormat->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))
	{
		format = 3;
	}
	if (IsEqualGUID(audioEngineFormat->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))
	{
		format = 1;
	}
	return format;
}
void setupEndpoint(State* state)
{
	HRESULT result;
	result = CoInitializeEx(0, 0);
	assert(result == S_OK);

	IMMDeviceEnumerator* audioEndpointEnumerator = {}; 
    result = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&audioEndpointEnumerator);
    assert(result == S_OK);

	IMMDevice* audioEndpoint = 0;
    result = audioEndpointEnumerator->lpVtbl->GetDefaultAudioEndpoint(audioEndpointEnumerator, eRender, eConsole, &audioEndpoint);
    assert(result == S_OK);

	IAudioClient* audioClient = {};
	result = audioEndpoint->lpVtbl->Activate(audioEndpoint, IID_IAudioClient, CLSCTX_ALL, 0, (void**)&audioClient);
	assert(result == S_OK);
	state->audioClient = audioClient;

	WAVEFORMATEXTENSIBLE* audioEngineFormat = {};
	result = audioClient->lpVtbl->GetMixFormat(audioClient, (WAVEFORMATEX**)&audioEngineFormat);
	assert(result == S_OK);

	audioEngineFormat->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
    state->format.type = 3;

	state->format.channelCount = 2;
    audioEngineFormat->Format.nChannels = 2;

	state->format.sampleRate = 48000;
    audioEngineFormat->Format.nSamplesPerSec = 48000;
	state->format.byteRate = 48000 * 2 * 4;
    audioEngineFormat->Format.nAvgBytesPerSec = 48000 * 2 * 4;
	state->format.blockAlign = 8;
    audioEngineFormat->Format.nBlockAlign = 8;
	state->format.bitDepth = 32;
    audioEngineFormat->Format.wBitsPerSample = 32;

	uint bufferFrameCount = state->frameCount;
	uint64 bufferDuration = (bufferFrameCount * 10000000ull) / 48000;
	bufferDuration *= 2;

	result = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, (REFERENCE_TIME)bufferDuration, 0, (WAVEFORMATEX*)audioEngineFormat, 0);
	assert(result == S_OK);

	HANDLE audioCallback = state->audioCallback;
	result = audioClient->lpVtbl->SetEventHandle(audioClient, audioCallback);
	assert(result == S_OK);

	IAudioRenderClient* audioRenderClient = {};
	result = audioClient->lpVtbl->GetService(audioClient, IID_IAudioRenderClient, (void**)&audioRenderClient);
	assert(result == S_OK);

	state->renderClient = audioRenderClient;

	IAudioClock* audioClock = {};
	result = audioClient->lpVtbl->GetService(audioClient, IID_IAudioClock, (void**)&audioClock);
	assert(result == S_OK);

	uint64 frequency;
	result = audioClock->lpVtbl->GetFrequency(audioClock, &frequency);
	assert(result == S_OK);

	state->audioClock = audioClock;
	state->endpointDeviceFrequency = frequency;
}
void sendLoadSignal(IAudioClient* audioClient, HANDLE* eventArray, uint eventCount, HANDLE endpointLoaderStartEvent, HANDLE endpointLoaderFinishEvent, uint frameCount)
{
	uint paddingFrameCount;
	audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
	if (paddingFrameCount < frameCount)
	{
		SetEvent(endpointLoaderStartEvent);
        setEventArray(eventArray, eventCount);
		WaitForSingleObject(endpointLoaderFinishEvent, INFINITE);
	}
}
DWORD WINAPI endpointController(LPVOID parameter)
{
	EndpointControllerInfo* endpointControllerInfo = (EndpointControllerInfo*)parameter;
	IAudioClient* audioClient = endpointControllerInfo->audioClient;
	HANDLE endpointLoaderStartEvent = endpointControllerInfo->endpointLoaderStartEvent;
	HANDLE endpointLoaderFinishEvent = endpointControllerInfo->endpointLoaderFinishEvent;
	ResetEvent(endpointLoaderFinishEvent);

    void* trackStartEventArrayHandle = endpointControllerInfo->trackStartEventArrayHandle;
    HANDLE* trackStartEventArray = {};
    uint eventCount = {};
    getArray(trackStartEventArrayHandle, (void**)&trackStartEventArray, &eventCount);

	HANDLE audioCallback = endpointControllerInfo->audioCallback;
    HANDLE exitSemaphore = endpointControllerInfo->exitSemaphore;
    HANDLE waitHandle[] = {audioCallback, exitSemaphore};
	uint frameCount = endpointControllerInfo->frameCount;
    uint running = 1;
	while (running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch (signal)
		{
			case WAIT_OBJECT_0:
			{
				sendLoadSignal(audioClient, trackStartEventArray, eventCount, endpointLoaderStartEvent, endpointLoaderFinishEvent, frameCount);
                break;
			}
			case WAIT_OBJECT_0 + 1:
			{
                freeMemory(endpointControllerInfo);
                running = 0;
            }
		}
	}
	return 0;
}
void loadSilence(IAudioRenderClient* renderClient, uint frameCount)
{
	float* audioEngineFrame = {};
	renderClient->lpVtbl->GetBuffer(renderClient, frameCount, (BYTE**)&audioEngineFrame);
	renderClient->lpVtbl->ReleaseBuffer(renderClient, frameCount, AUDCLNT_BUFFERFLAGS_SILENT);
}
DWORD WINAPI dummyLoader(LPVOID parameter)
{
	EndpointLoaderInfo* endpointLoaderInfo = (EndpointLoaderInfo*)parameter;
    HANDLE exitSemaphore = endpointLoaderInfo->exitSemaphore;
    IAudioRenderClient* renderClient = endpointLoaderInfo->renderClient;
	HANDLE endpointLoaderStartEvent = endpointLoaderInfo->endpointLoaderStartEvent;
	HANDLE endpointLoaderFinishEvent = endpointLoaderInfo->endpointLoaderFinishEvent;

	uint frameCount = endpointLoaderInfo->frameCount;
    HANDLE waitHandle[] = {endpointLoaderStartEvent, exitSemaphore};
    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
                loadSilence(renderClient, frameCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeMemory(endpointLoaderInfo);
                running = 0;
            }
        }
        SetEvent(endpointLoaderFinishEvent);
    }
    return 0;
}
void startEndpointController(State* state, void* trackStartEventArrayHandle)
{
    EndpointControllerInfo* endpointControllerInfo = {};
    allocateMemory(sizeof(EndpointControllerInfo), (void**)&endpointControllerInfo);
    endpointControllerInfo->audioClient = state->audioClient;
    endpointControllerInfo->endpointLoaderStartEvent = state->endpointLoaderStartEvent;
    endpointControllerInfo->endpointLoaderFinishEvent = state->endpointLoaderFinishEvent;
    endpointControllerInfo->audioCallback = state->audioCallback;
    endpointControllerInfo->exitSemaphore = state->exitSemaphore;
    endpointControllerInfo->frameCount = state->frameCount;
    endpointControllerInfo->trackStartEventArrayHandle = trackStartEventArrayHandle;

    createThread(endpointController, endpointControllerInfo);
}
void loadOutput(IAudioRenderClient* renderClient, float* inputBuffer, uint iterationCount, uint frameCount)
{
	float* audioEngineFrame = {};
	HRESULT result;
	result = renderClient->lpVtbl->GetBuffer(renderClient, frameCount, (BYTE**)&audioEngineFrame);
	copySample(&inputBuffer, audioEngineFrame, iterationCount);
	renderClient->lpVtbl->ReleaseBuffer(renderClient, frameCount, 0);
}
DWORD WINAPI endpointLoader(LPVOID parameter)
{
	EndpointLoaderInfo* endpointLoaderInfo = (EndpointLoaderInfo*)parameter;
    float* buffer = endpointLoaderInfo->buffer;

	uint frameCount = endpointLoaderInfo->frameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;

    IAudioRenderClient* renderClient = endpointLoaderInfo->renderClient;
    HANDLE processorStartEvent = endpointLoaderInfo->processorStartEvent;
    HANDLE processorFinishEvent = endpointLoaderInfo->processorFinishEvent;

	HANDLE endpointLoaderStartEvent = endpointLoaderInfo->endpointLoaderStartEvent;
	HANDLE endpointLoaderFinishEvent = endpointLoaderInfo->endpointLoaderFinishEvent;
    HANDLE exitSemaphore = endpointLoaderInfo->exitSemaphore;
    HANDLE waitHandle[] = {endpointLoaderStartEvent, exitSemaphore};
    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
                WaitForSingleObject(processorFinishEvent, INFINITE);
                loadOutput(renderClient, buffer, iterationCount, frameCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeMemory(endpointLoaderInfo);
                running = 0;
            }
        }
        SetEvent(processorStartEvent);
        SetEvent(endpointLoaderFinishEvent);
    }
	return 0;
}
DWORD WINAPI endpointProcessor(LPVOID parameter)
{
    EndpointProcessorInfo* endpointProcessorInfo = (EndpointProcessorInfo*)parameter;
    uint inputLoaderCount = endpointProcessorInfo->inputLoaderCount;
    float* buffer = endpointProcessorInfo->buffer;
    float* endpointBuffer = endpointProcessorInfo->endpointBuffer; 

	uint frameCount = endpointProcessorInfo->frameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;

    HANDLE processorStartEvent = endpointProcessorInfo->processorStartEvent;
    HANDLE processorFinishEvent = endpointProcessorInfo->processorFinishEvent;
    HANDLE finishSemaphore = endpointProcessorInfo->finishSemaphore;
    HANDLE exitSemaphore = endpointProcessorInfo->exitSemaphore;
    SetEvent(processorStartEvent);
    HANDLE waitHandle[] = {finishSemaphore, exitSemaphore};

    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
				checkCompletion(finishSemaphore, inputLoaderCount);
                WaitForSingleObject(processorStartEvent, INFINITE);
                accumulateFrame(buffer, endpointBuffer, iterationCount, inputLoaderCount);
                //process(inputBuffer, iterationCount, inputLoaderCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeMemory(endpointProcessorInfo);
                running = 0;
            }
        }
        SetEvent(processorFinishEvent);
    }
	return 0;
}
void startDummyLoader(State* state)
{
    EndpointLoaderInfo* endpointLoaderInfo = {};
    allocateMemory(sizeof(EndpointLoaderInfo), (void**)&endpointLoaderInfo);
    endpointLoaderInfo->renderClient = state->renderClient;
    endpointLoaderInfo->endpointLoaderStartEvent = state->endpointLoaderStartEvent;
    endpointLoaderInfo->endpointLoaderFinishEvent = state->endpointLoaderFinishEvent;
    endpointLoaderInfo->exitSemaphore = state->exitSemaphore;
    endpointLoaderInfo->frameCount = state->frameCount;

    createThread(dummyLoader, endpointLoaderInfo);
}
void startEndpointProcessor(State* state)
{
    EndpointProcessorInfo* endpointProcessorInfo = {};
    allocateMemory(sizeof(EndpointProcessorInfo), (void**)&endpointProcessorInfo);
    endpointProcessorInfo->processorStartEvent = state->processorStartEvent;
    endpointProcessorInfo->processorFinishEvent = state->processorFinishEvent;
    endpointProcessorInfo->finishSemaphore = state->finishSemaphore;
    endpointProcessorInfo->exitSemaphore = state->exitSemaphore;
    endpointProcessorInfo->inputLoaderCount = state->inputLoaderCount;
    endpointProcessorInfo->frameCount = state->frameCount;
    endpointProcessorInfo->buffer = state->buffer;
    endpointProcessorInfo->endpointBuffer = state->endpointBuffer;

    createThread(endpointProcessor, endpointProcessorInfo);
}
void startEndpointLoader(State* state)
{
    EndpointLoaderInfo* endpointLoaderInfo = {};
    allocateMemory(sizeof(EndpointLoaderInfo), (void**)&endpointLoaderInfo);
    endpointLoaderInfo->processorStartEvent = state->processorStartEvent;
    endpointLoaderInfo->processorFinishEvent = state->processorFinishEvent;
    endpointLoaderInfo->endpointLoaderStartEvent = state->endpointLoaderStartEvent;
    endpointLoaderInfo->endpointLoaderFinishEvent = state->endpointLoaderFinishEvent;
    endpointLoaderInfo->finishSemaphore = state->finishSemaphore;
    endpointLoaderInfo->exitSemaphore = state->exitSemaphore;
    endpointLoaderInfo->renderClient = state->renderClient;
    endpointLoaderInfo->buffer = state->endpointBuffer;
    endpointLoaderInfo->frameCount = state->frameCount;

    createThread(endpointLoader, endpointLoaderInfo);
}
void startPlayback(State* state, HWND window, WPARAM wParam)
{
    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    uint trackCount = {};
    getOccupiedSlotCount(inputTrackArrayHandle, &trackCount);
    if(trackCount)
    {
        startEndpointProcessor(state);
        startEndpointLoader(state);
    }
    else
    {
        startDummyLoader(state);
    }

	IAudioClient* audioClient = state->audioClient;
    HWND clipAreaWindow = state->clipAreaWindow;
	SendMessage(clipAreaWindow, WM_PLAY, 0, 0);

	audioClient->lpVtbl->Start(audioClient);
	SetTimer(window, 1, 15, 0);
}
void flushBuffer(State* state)
{
	IAudioClient* audioClient = state->audioClient;
	audioClient->lpVtbl->Start(audioClient); 
	uint paddingFrameCount = 1;
	while(paddingFrameCount != 0)
	{
		audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
		Sleep(1);
	}
	audioClient->lpVtbl->Stop(audioClient); 
	audioClient->lpVtbl->Reset(audioClient); 
}
void stopPlayback(State* state, HWND window)
{
	IAudioClient* audioClient = state->audioClient;
	audioClient->lpVtbl->Stop(audioClient); 

    long threadCount = 2;
	if(state->inputLoaderCount)
	{
        threadCount = 3;
	    state->inputLoaderCount = 0;
		freeMemory(state->buffer);
		freeMemory(state->endpointBuffer);
        
	}
    HANDLE exitSemaphore = state->exitSemaphore;
    ReleaseSemaphore(exitSemaphore, threadCount, 0);
    waitForSemaphore(exitSemaphore);

	flushBuffer(state);
	KillTimer(window, 1);

    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    resetArray(inputTrackArrayHandle);
    state->inputLoaderCount = 0;
}
void handleTimer(State* state)
{
    IAudioClock* audioClock = state->audioClock;
	uint64 position;
	uint64 timeStamp;
	audioClock->lpVtbl->GetPosition(audioClock, &position, &timeStamp);
	uint64 frequency = state->endpointDeviceFrequency;

	position *= 1000;
	uint64 timeElapsedInMilliseconds = position / frequency;
    uint framesPerMilliseconds = state->format.sampleRate / 1000;
    uint64 framesElapsed = timeElapsedInMilliseconds * framesPerMilliseconds;

	HWND clipAreaWindow = state->clipAreaWindow;
	SendMessage(clipAreaWindow, WM_TIMER, (WPARAM)framesElapsed, 0);
}
void setInput(State* state, WPARAM wParam)
{
    HWND inputTrack = (HWND)wParam;
    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    appendElement(inputTrackArrayHandle, (char*)&inputTrack);
    ++state->inputLoaderCount;
}
void prepareToPlay(State* state, WPARAM wParam)
{
    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    HWND* trackArray = {};
    uint trackCount = {};
    getArray(inputTrackArrayHandle, (void**)&trackArray, &trackCount);

    void* trackStartEventArrayHandle = (void*)wParam;
    startEndpointController(state, trackStartEventArrayHandle);
    if(!trackCount)
    {
        return;
    }

    uint frameCount = state->frameCount;
    float* endpointBuffer = {};
    allocateMemory(frameCount * sizeof(float) * 2, (void**)&endpointBuffer);
    state->endpointBuffer = endpointBuffer;

    float* buffer = {};
    uint bufferSize = frameCount * trackCount * sizeof(float) * 2;
    allocateMemory(bufferSize, (void**)&buffer);
    state->buffer = buffer;

    BufferLoaderInfo loaderInfo = {};
    loaderInfo.outputBuffer = state->buffer;
    loaderInfo.finishSemaphore = state->finishSemaphore;
    loaderInfo.loaderCount = state->inputLoaderCount;
    loaderInfo.frameCount = state->frameCount;

    startBufferLoader(trackArray, trackCount, &loaderInfo);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_CREATECHILD:
        {
            setupEndpoint(state);
            break;
        }
        case WM_PLAY:
        {
            startPlayback(state, window, wParam);
            break;
        }
        case WM_PAUSE:
        {
            stopPlayback(state, window);
            break;
        }
        case WM_SETINPUT:
        {
            setInput(state, wParam);
            break;
        }
        case WM_PREPARETOPLAY:
        {
            prepareToPlay(state, wParam);
            break;
        }
        case WM_TIMER:
        {
            handleTimer(state);
            break;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
