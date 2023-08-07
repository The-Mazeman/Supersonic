#include "header.h"
#include "wasapi.h"

START_SCOPE(wasapi)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* wasapi)
{
	createWindowClass(L"wasapiWindowClass", windowCallback);
	createChildWindow(L"wasapiWindowClass", window, wasapi);
}
uint getAudioEngineSubFormat(WAVEFORMATEXTENSIBLE* audioEngineFormat)
{
	uint format = 0;
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

	state->format.type = (ushort)getAudioEngineSubFormat(audioEngineFormat);
    state->format.channelCount = audioEngineFormat->Format.nChannels;
    state->format.sampleRate = audioEngineFormat->Format.nSamplesPerSec;
    state->format.byteRate = audioEngineFormat->Format.nAvgBytesPerSec;
    state->format.blockAlign = audioEngineFormat->Format.nBlockAlign;
    state->format.bitDepth = audioEngineFormat->Format.wBitsPerSample;

	result = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 426700, 0, (WAVEFORMATEX*)audioEngineFormat, 0);
	assert(result == S_OK);

	HANDLE audioCallback = state->audioCallback;
	result = audioClient->lpVtbl->SetEventHandle(audioClient, audioCallback);
	assert(result == S_OK);


	uint32 bufferFrameCount;
	result = audioClient->lpVtbl->GetBufferSize(audioClient, &bufferFrameCount);
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
void setCursor(State* state, WPARAM wParam)
{
	HWND cursor = (HWND)wParam;
	state->cursor = cursor;
}
void sendLoadSignal(IAudioClient* audioClient, HANDLE loadEvent, uint frameCount)
{
	uint paddingFrameCount;
	audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
	if (paddingFrameCount < frameCount)
	{
		SetEvent(loadEvent);
	}
}
DWORD WINAPI endpointController(LPVOID parameter)
{
	State* state = (State*)parameter;
	IAudioClient* audioClient = state->audioClient;
	HANDLE loadEvent = state->outputLoadEvent;
	HANDLE audioCallback = state->audioCallback;
	uint frameCount = state->bufferFrameCount / 2;
	while(1)
	{
		uint signal = WaitForSingleObject(audioCallback, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{
				sendLoadSignal(audioClient, loadEvent, frameCount);
			}
		}
	}
	return 0;
}
void load(float** source, float** destination, uint iterationCount)
{
	__m256* sourceAVX = (__m256*) * source;
	__m256* destinationAVX = (__m256*) * destination;
	for (uint i = 0; i != iterationCount; ++i)
	{
		*destinationAVX = *sourceAVX;
		++destinationAVX;
		++sourceAVX;
	}
	*source = (float*)sourceAVX;
}
void loadOutput(IAudioRenderClient* renderClient, float** outputBuffer, uint iterationCount, uint frameCount)
{
	float* audioEngineFrame = {};
	HRESULT result;
	result = renderClient->lpVtbl->GetBuffer(renderClient, frameCount, (BYTE**)&audioEngineFrame);
	load(outputBuffer, &audioEngineFrame, iterationCount);
	renderClient->lpVtbl->ReleaseBuffer(renderClient, frameCount, 0);
}
DWORD WINAPI endpointLoader(LPVOID parameter)
{
	State* state = (State*)parameter;

	HANDLE outputLoadEvent = state->outputLoadEvent;
	HANDLE inputLoadEvent = state->inputLoadEvent;
	HANDLE exitEvent = state->exitLoader;
	HANDLE inputExitEvent = state->inputExitEvent;
	HANDLE inputFinishSemaphore = state->inputFinishSemaphore;
	uint* inputFinishCount = &state->inputFinishCount;

	IAudioRenderClient* renderClient = state->renderClient;
	RingBuffer* inputBuffer = &state->endpointBuffer;
	float* buffer = (float*)inputBuffer->start;
	uint loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;
	HANDLE waitHandle[] = {exitEvent, outputLoadEvent};
	uint running = 1;
	while(running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		checkCompletion(inputFinishCount, 1, inputFinishSemaphore);
		switch(signal) 
		{
			case WAIT_OBJECT_0:
			{
				running = 0;
				SetEvent(inputExitEvent);
				checkCompletion(inputFinishCount, 1, inputFinishSemaphore);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				loadOutput(renderClient, &buffer, iterationCount, loadFrameCount);
				boundCheck(inputBuffer, (void**)&buffer, 0);
				SetEvent(inputLoadEvent);
			}
		}
	}
	return 0;
}
void startPlayback(State* state, HWND window)
{
	createThread(endpointLoader, state, 0);

	IAudioClient* audioClient = state->audioClient;
	HWND cursor = state->cursor;
	SendMessage(cursor, WM_PLAY, 0, 0);
	audioClient->lpVtbl->Start(audioClient);
	SetTimer(window, 1, 15, 0);
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
	SetEvent(state->exitLoader);

	flushBuffer(state);
	IAudioClient* audioClient = state->audioClient;
	audioClient->lpVtbl->Stop(audioClient); 
	audioClient->lpVtbl->Reset(audioClient); 

	KillTimer(window, 1);
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
void createOutputBuffer(State* state)
{
	uint bufferFrameCount = state->bufferFrameCount;
	uint bufferFrameSize = state->format.blockAlign;
	uint bufferMemorySize = bufferFrameCount * bufferFrameSize;

	char* buffer;
	allocateSmallMemory(bufferMemorySize, (void**)&buffer);

	state->endpointBuffer.start = buffer;
	state->endpointBuffer.end = buffer + bufferMemorySize;
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	createEvent(0, &state->audioCallback);
	createEvent(0, &state->inputLoadEvent);
	createEvent(0, &state->outputLoadEvent);
	createEvent(0, &state->inputExitEvent);
	createEvent(0, &state->exitLoader);
	createSemaphore(0, 3, &state->inputFinishSemaphore);

    setupEndpoint(state);
	createOutputBuffer(state);

	createThread(endpointController, state, 0);

	bus::create(window, &state->busArray[0]);
}
void startLoader(State* state)
{
	Loader busLoader = {};
	busLoader.buffer = state->endpointBuffer;
	busLoader.loadEvent = state->inputLoadEvent;
	busLoader.exitSemaphore = state->inputExitEvent;
	busLoader.finishSemaphore = state->inputFinishSemaphore;
	busLoader.finishCount = &state->inputFinishCount;
	busLoader.trackCount = 1;
	busLoader.trackNumber = 0;

	HWND masterBus = state->busArray[0];
	SendMessage(masterBus, WM_STARTLOADER, (WPARAM)&busLoader, 0);

	for(uint i = 0; i != 2; ++i)
	{
		SetEvent(state->inputLoadEvent);
		WaitForSingleObject(state->inputFinishSemaphore, INFINITE);
	}
	ReleaseSemaphore(state->inputFinishSemaphore, 1, 0);
	state->inputFinishCount = 1;
	SetEvent(state->outputLoadEvent);
}
void assignBus(State* state, WPARAM wParam, LPARAM lParam)
{
	HWND bus = state->busArray[lParam];
	SendMessage(bus, WM_ASSIGNBUS, wParam, 0);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
            initialize(window);
			break;
		}
		case WM_SETCURSOR:
		{
			setCursor(state, wParam);
			break;
		}
		case WM_TIMER:
		{
			handleTimer(state);
			break;
		}
		case WM_STARTLOADER:
		{
			startLoader(state);
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
		case WM_ASSIGNBUS:
		{
			assignBus(state, wParam, lParam);
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
