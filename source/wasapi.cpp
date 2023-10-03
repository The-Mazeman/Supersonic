#include "header.h"
#include "wasapi.h"

START_SCOPE(wasapi)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

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

	IMMDeviceEnumerator* audioEndpointEnumerator = (IMMDeviceEnumerator*)1;
	if (audioEndpointEnumerator)
	{
		result = CoCreateInstance(CLSID_MMDeviceEnumerator, 0, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&audioEndpointEnumerator);
		assert(result == S_OK);
	}

	IMMDevice* audioEndpoint = (IMMDevice*)1;
	if (audioEndpointEnumerator)
	{
		result = audioEndpointEnumerator->lpVtbl->GetDefaultAudioEndpoint(audioEndpointEnumerator, eRender, eConsole, &audioEndpoint);
		assert(result == S_OK);
	}

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

	uint bufferFrameCount = 1024;
	uint64 bufferDuration = (bufferFrameCount * 10000000ull) / 48000;
	bufferDuration *= 2;

	result = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, (REFERENCE_TIME)bufferDuration, 0, (WAVEFORMATEX*)audioEngineFormat, 0);
	assert(result == S_OK);

	HANDLE audioCallback = state->audioCallback;
	result = audioClient->lpVtbl->SetEventHandle(audioClient, audioCallback);
	assert(result == S_OK);

	state->bufferFrameCount = bufferFrameCount;
	globalState.audioEndpointFrameCount = bufferFrameCount;

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
void createInputBuffer(State* state)
{
	uint bufferFrameCount = state->bufferFrameCount;
	uint bufferFrameSize = state->format.blockAlign;
	uint bufferMemorySize = bufferFrameCount * bufferFrameSize;

	float* buffer = {};
	allocateSmallMemory(bufferMemorySize, (void**)&buffer);

	state->inputBuffer = buffer;
}
void sendLoadSignal(IAudioClient* audioClient, HANDLE endpointLoaderFinishEvent, HANDLE loadEvent, uint frameCount)
{
	uint paddingFrameCount;
	audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
	if (paddingFrameCount < frameCount)
	{
		SetEvent(loadEvent);
		WaitForSingleObject(endpointLoaderFinishEvent, INFINITE);
	}
}
DWORD WINAPI endpointController(LPVOID parameter)
{
	EndpointControllerInfo* controller = (EndpointControllerInfo*)parameter;
	IAudioClient* audioClient = controller->audioClient;
	HANDLE loadEvent = controller->endpointLoaderStartEvent;
	HANDLE endpointLoaderFinishEvent = controller->endpointLoaderFinishEvent;
	ResetEvent(endpointLoaderFinishEvent);
	HANDLE audioCallback = controller->audioCallback;
    HANDLE exitSemaphore = controller->exitSemaphore;
    HANDLE waitHandle[] = {audioCallback, exitSemaphore};
	uint frameCount = globalState.audioEndpointFrameCount;
    uint running = 1;
	while (running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch (signal)
		{
			case WAIT_OBJECT_0:
			{
				sendLoadSignal(audioClient, endpointLoaderFinishEvent, loadEvent, frameCount);
                break;
			}
			case WAIT_OBJECT_0 + 1:
			{
                freeSmallMemory(controller);
                running = 0;
            }
		}
	}
	return 0;
}
void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->audioEngine = parent;
    state->inputLoaderCount = 0;
    createEvent(0, &state->audioCallback);
    createEvent(0, &state->endpointLoaderStartEvent);
    createEvent(0, &state->endpointLoaderFinishEvent);
    createEvent(0, &state->outputLoaderStartEvent);
    createSemaphore(0, 2, &state->exitSemaphore);
	createSemaphore(0, 3, &state->inputBufferCompleteSemaphore);

	setupEndpoint(state);
	createWindowClass(L"wasapiWindowClass", windowCallback);
	createChildWindow(L"wasapiWindowClass", parent, window, state);
}
void setCursor(State* state, WPARAM wParam)
{
	HWND cursor = (HWND)wParam;
	state->cursor = cursor;
}
void load(float* source, float* destination, uint iterationCount)
{
	__m256* sourceAVX = (__m256*)source;
	__m256* destinationAVX2 = (__m256*)destination;
	for (uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, *sourceAVX);
		++destinationAVX2;
		++sourceAVX;
	}
}
void loadOutput(IAudioRenderClient* renderClient, float* inputBuffer, uint iterationCount, uint frameCount)
{
	float* audioEngineFrame = {};
	HRESULT result;
	result = renderClient->lpVtbl->GetBuffer(renderClient, frameCount, (BYTE**)&audioEngineFrame);
	load(inputBuffer, audioEngineFrame, iterationCount);
	renderClient->lpVtbl->ReleaseBuffer(renderClient, frameCount, 0);
}
void setTrackEvent(HANDLE* eventArray, uint trackCount)
{
	for (uint i = 0; i != trackCount; ++i)
	{
		SetEvent(eventArray[i]);
	}
}
DWORD WINAPI endpointLoader(LPVOID parameter)
{
	EndpointLoaderInfo* endpointLoaderInfo = (EndpointLoaderInfo*)parameter;
    float* inputBuffer = endpointLoaderInfo->inputBuffer;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

    IAudioRenderClient* renderClient = endpointLoaderInfo->renderClient;
    HANDLE outputLoaderStartEvent = endpointLoaderInfo->outputLoaderStartEvent;
    HANDLE completeSemaphore = endpointLoaderInfo->bufferCompleteSemaphore;
    HANDLE* trackProcessorStartEventArray = endpointLoaderInfo->trackProcessorStartEventArray;
    uint trackCount = endpointLoaderInfo->trackCount;

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
				WaitForSingleObject(completeSemaphore, INFINITE);
                setEventArray(trackProcessorStartEventArray, trackCount);
                loadOutput(renderClient, inputBuffer, iterationCount, loadFrameCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeSmallMemory(endpointLoaderInfo);
                running = 0;
            }
        }
        SetEvent(outputLoaderStartEvent);
        SetEvent(endpointLoaderFinishEvent);
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
    HANDLE endpointLoaderStartEvent = endpointLoaderInfo->endpointLoaderStartEvent;
    HANDLE endpointLoaderFinishEvent = endpointLoaderInfo->endpointLoaderFinishEvent;
    HANDLE exitSemaphore = endpointLoaderInfo->exitSemaphore;
    IAudioRenderClient* renderClient = endpointLoaderInfo->renderClient;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
    HANDLE waitHandle[] = {endpointLoaderStartEvent, exitSemaphore};
    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
                loadSilence(renderClient, loadFrameCount);
				SetEvent(endpointLoaderFinishEvent);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeSmallMemory(endpointLoaderInfo);
                running = 0;
            }
        }
    }
    return 0;
}
void startInputLoader(State* state)
{
	BufferInfo bufferInfo = {};
	bufferInfo.buffer = state->inputBuffer;
	bufferInfo.bufferCompleteSemaphore = state->inputBufferCompleteSemaphore;
	bufferInfo.loaderCount = state->inputLoaderCount;

    HANDLE outputLoaderStartEvent = state->outputLoaderStartEvent;
    HWND masterBus = state->inputLoaderArray[0];
    SendMessage(masterBus, WM_STARTOUTPUTLOADER, (WPARAM)&bufferInfo, (LPARAM)outputLoaderStartEvent);
}
void startPlayback(State* state, HWND window)
{
	notUsing(window);
	IAudioClient* audioClient = state->audioClient;
	HWND cursor = state->cursor;
	SendMessage(cursor, WM_PLAY, 0, 0);
	audioClient->lpVtbl->Start(audioClient);
	//SetTimer(window, 1, 15, 0);
}
void flushBuffer(State* state)
{
	uint paddingFrameCount = 1;
	IAudioClient* audioClient = state->audioClient;
	while(paddingFrameCount != 0)
	{
		audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
		Sleep(1);
	}
}
void stopPlayback(State* state, HWND window)
{
	IAudioClient* audioClient = state->audioClient;
	audioClient->lpVtbl->Stop(audioClient); 

    HANDLE exitSemaphore = state->exitSemaphore;
    ReleaseSemaphore(exitSemaphore, 2, 0);
    waitForSemaphore(exitSemaphore);

	audioClient->lpVtbl->Start(audioClient); 
	flushBuffer(state);
	audioClient->lpVtbl->Stop(audioClient); 
	audioClient->lpVtbl->Reset(audioClient); 

	KillTimer(window, 1);
	if(state->inputLoaderCount)
	{
		state->inputLoaderCount = 0;
		freeSmallMemory(state->inputBuffer);
	}
}
void handleTimer(State* state)
{
	IAudioClock* audioClock = state->audioClock;
	uint64 position;
	uint64 timeStamp;
	audioClock->lpVtbl->GetPosition(audioClock, &position, &timeStamp);

	uint64 frequency = state->endpointDeviceFrequency;

	position *= 1000;
	sint64 timeElapsedInMilliseconds = (sint64)(position / frequency);

	HWND cursor = state->cursor;
	SendMessage(cursor, WM_TIMER, (WPARAM)timeElapsedInMilliseconds, 0);
}
void startOutputLoader(State* state, WPARAM wParam, LPARAM lParam)
{
	EndpointControllerInfo* endpointControllerInfo = {};
    allocateSmallMemory(sizeof(EndpointControllerInfo), (void**)&endpointControllerInfo);
	endpointControllerInfo->audioCallback = state->audioCallback;
	endpointControllerInfo->audioClient = state->audioClient;
	endpointControllerInfo->endpointLoaderStartEvent = state->endpointLoaderStartEvent;
	endpointControllerInfo->endpointLoaderFinishEvent = state->endpointLoaderFinishEvent;
	endpointControllerInfo->exitSemaphore = state->exitSemaphore;

    createThread(endpointController, endpointControllerInfo, 0);

	EndpointLoaderInfo* endpointLoaderInfo = {};
    allocateSmallMemory(sizeof(EndpointLoaderInfo), (void**)&endpointLoaderInfo);
	endpointLoaderInfo->endpointLoaderStartEvent = state->endpointLoaderStartEvent;
	endpointLoaderInfo->endpointLoaderFinishEvent = state->endpointLoaderFinishEvent;
	endpointLoaderInfo->exitSemaphore = state->exitSemaphore;
	endpointLoaderInfo->renderClient = state->renderClient;

    uint inputLoaderCount = state->inputLoaderCount;
    if(inputLoaderCount == 0)
    {
        createThread(dummyLoader, endpointLoaderInfo, 0);
        return;
    }

    createInputBuffer(state);

	endpointLoaderInfo->inputBuffer = state->inputBuffer;
	endpointLoaderInfo->bufferCompleteSemaphore = state->inputBufferCompleteSemaphore;
	endpointLoaderInfo->trackProcessorStartEventArray = (HANDLE*)wParam;
	endpointLoaderInfo->trackCount = (uint)lParam;
	endpointLoaderInfo->outputLoaderStartEvent = state->outputLoaderStartEvent;

    createThread(endpointLoader, endpointLoaderInfo, 0);
}
void setInputLoader(State* state, WPARAM wParam)
{
	uint inputLoaderCount = state->inputLoaderCount;
	state->inputLoaderArray[inputLoaderCount] = (HWND)wParam;
	++state->inputLoaderCount;
}
void sendInputLoader(State* state)
{
	HWND* inputLoaderArray = state->inputLoaderArray;
	uint inputLoaderCount = state->inputLoaderCount;
	for (uint i = 0; i != inputLoaderCount; ++i)
	{
		SendMessage(inputLoaderArray[i], WM_SETOUTPUTLOADER, 0, 0);
	}
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
			setState(window, lParam);
			break;
		}
		case WM_SETCURSOR:
		{
			setCursor(state, wParam);
			break;
		}
		case WM_SENDINPUTLOADER:
		{
			sendInputLoader(state);
			break;
		}
		case WM_SETINPUTLOADER:
		{
			setInputLoader(state, wParam);
			break;
		}
		case WM_TIMER:
		{
			handleTimer(state);
			break;
		}
		case WM_STARTINPUTLOADER:
        {
            startInputLoader(state);
            break;
        }
		case WM_STARTOUTPUTLOADER:
		{
			startOutputLoader(state, wParam, lParam);
			break;
		}
		case WM_PLAY:
		{
			startPlayback(state, window);
			break;
		}
		case WM_PAUSE:
		{
			stopPlayback(state, window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
