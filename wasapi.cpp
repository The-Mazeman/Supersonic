#include "header.h"
#include "wasapi.h"
#include "globalState.h"

START_SCOPE(wasapi)

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
void setupEndpoint(HWND window)
{
	State* state = (State*)GetProp(window, L"state");

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

	Header format = {};
	format.type = (ushort)getAudioEngineSubFormat(audioEngineFormat);
	format.channelCount = audioEngineFormat->Format.nChannels;
	format.sampleRate = audioEngineFormat->Format.nSamplesPerSec;
	format.byteRate = audioEngineFormat->Format.nAvgBytesPerSec;
	format.blockAlign = audioEngineFormat->Format.nBlockAlign;
	format.bitDepth = audioEngineFormat->Format.wBitsPerSample;

	state->format = format;
	result = audioClient->lpVtbl->Initialize(audioClient, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 426700, 0, (WAVEFORMATEX*)audioEngineFormat, 0);
	assert(result == S_OK);

	HANDLE audioCallback;
	createEvent(0, &audioCallback);
	result = audioClient->lpVtbl->SetEventHandle(audioClient, audioCallback);
	assert(result == S_OK);

	state->audioCallback = audioCallback;

	uint32 bufferFrameCount;
	result = audioClient->lpVtbl->GetBufferSize(audioClient, &bufferFrameCount);
	assert(result == S_OK);

	state->bufferFrameCount = bufferFrameCount;

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
void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	state->exitCount = 0;

	HANDLE exitSemaphore;
	createSemaphore(0, 2, &exitSemaphore);
	state->exitSemaphore = exitSemaphore;

	HANDLE loadEvent;
	createEvent(0, &loadEvent);
	state->loadEvent = loadEvent;

	HWND parent = GetAncestor(window, GA_PARENT);
	SendMessage(parent, WM_SETCALLBACK, (WPARAM)loadEvent, 0);
	
	SetProp(window, L"state", state);
}
void setCursor(HWND window, WPARAM wParam)
{
	HWND cursor = (HWND)wParam;
	State* state = (State*)GetProp(window, L"state");
	state->cursor = cursor;
}
void sendLoadSignal(IAudioClient* audioClient, HANDLE loadEvent, uint frameCount)
{
	uint paddingFrameCount;
	audioClient->lpVtbl->GetCurrentPadding(audioClient, &paddingFrameCount);
	if (frameCount >= paddingFrameCount)
	{
		SetEvent(loadEvent);
	}
}
DWORD WINAPI endpointController(LPVOID parameter)
{
	State* state = (State*)parameter;
	IAudioClient* audioClient = state->audioClient;

	HANDLE loadEvent = state->loadEvent;
	HANDLE audioCallback = state->audioCallback;
	HANDLE exitSemaphore = state->exitSemaphore;
	HANDLE waitHandle[] = {exitSemaphore , audioCallback};
	uint frameCount = state->bufferFrameCount / 2;
	uint* exitCount = &state->exitCount;
	uint running = 1;
	while (running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{
				InterlockedIncrement(exitCount);
				running = 0;
				break;
			}
			case WAIT_OBJECT_0 + 1:
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
	renderClient->lpVtbl->GetBuffer(renderClient, frameCount, (BYTE**)&audioEngineFrame);
	load(outputBuffer, &audioEngineFrame, iterationCount);
	renderClient->lpVtbl->ReleaseBuffer(renderClient, frameCount, 0);
}
void bufferBoundCheck(RingBuffer* ringBuffer, void** bufferPointer, uint offset)
{
	char* bufferStart = ringBuffer->start + offset;
	char* bufferEnd = ringBuffer->end + offset;
	if (*bufferPointer == bufferEnd)
	{
		*bufferPointer = bufferStart;
	}
}
DWORD WINAPI endpointLoader(LPVOID parameter)
{
	State* state = (State*)parameter;

	HANDLE loadEvent = state->loadEvent;
	HANDLE exitSemaphore = state->exitSemaphore;

	IAudioRenderClient* renderClient = state->renderClient;
	RingBuffer* outputBuffer = &state->outputBuffer;
	float* outputBufferPointer = (float*)outputBuffer->start;
	uint frameCount = globalState.audioFrameCount / 2;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;
	HANDLE waitHandle[] = {exitSemaphore, loadEvent};
	uint* exitCount = &state->exitCount;
	uint running = 1;
	while(running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch(signal) 
		{
			case WAIT_OBJECT_0:
			{
				running = 0;
				InterlockedIncrement(exitCount);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				loadOutput(renderClient, &outputBufferPointer, iterationCount, frameCount);
				bufferBoundCheck(outputBuffer, (void**)&outputBufferPointer, 0);
			}
		}
	}
	return 0;
}
void startPlayback(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	IAudioClient* audioClient = state->audioClient;
	audioClient->lpVtbl->Start(audioClient);

	HWND cursor = state->cursor;
	SendMessage(cursor, WM_PLAY, 0, 0);

	createThread(endpointController, state, 0);
	createThread(endpointLoader, state, 0);
	SetTimer(window, 1, 15, 0);
}
void stopPlayback(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	HANDLE exitSemaphore = state->exitSemaphore;
	IAudioClient* audioClient = state->audioClient;

	KillTimer(window, 1);
	ReleaseSemaphore(exitSemaphore, 2, 0);
	uint* exitCount = &state->exitCount;
	while(*exitCount != 2)
	{
		Sleep(1);
	}
	*exitCount = 0;

	audioClient->lpVtbl->Stop(audioClient); 
	audioClient->lpVtbl->Reset(audioClient); 

	freeSmallMemory(state->outputBuffer.start);
}
void handleTimer(HWND window)
{
	State* state = (State*)GetProp(window, L"state");

	IAudioClock* audioClock = state->audioClock;
	uint64 position;
	uint64 timeStamp;
	audioClock->lpVtbl->GetPosition(audioClock, &position, &timeStamp);

	uint64 frequency = state->endpointDeviceFrequency;

	position *= 1000;
	sint64 timeElapsedInMilliseconds = (sint64)(position / frequency);

	HWND cursor = state->cursor;
	SendMessage(cursor, WM_TIMER, 0, timeElapsedInMilliseconds);
}
void createOutputBuffer(State* state)
{
	uint bufferFrameCount = state->bufferFrameCount;
	uint bufferFrameSize = (uint)(state->format.bitDepth * state->format.channelCount);
	uint bufferMemorySize = bufferFrameCount * bufferFrameSize;

	char* buffer;
	allocateSmallMemory(bufferMemorySize, &buffer);

	state->outputBuffer.start = buffer;
	state->outputBuffer.end = buffer + bufferMemorySize;
	globalState.audioFrameCount = bufferFrameCount;
}
void setOutput(HWND window, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	createOutputBuffer(state);

	RingBuffer** outputBuffer = (RingBuffer**)wParam;
	*outputBuffer = &state->outputBuffer;
	
	HANDLE* loadEvent = (HANDLE*)lParam;
	*loadEvent = state->loadEvent;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
			setupEndpoint(window);
			break;
		}
		case WM_SETCURSOR:
		{
			setCursor(window, wParam);
			break;
		}
		case WM_GETOUTPUT:
		{
			setOutput(window, wParam, lParam);
			break;
		}
		case WM_TIMER:
		{
			handleTimer(window);
			break;
		}
		case WM_PLAY:
		{
			startPlayback(window);
			break;
		}
		case WM_PAUSE:
		{
			stopPlayback(window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE