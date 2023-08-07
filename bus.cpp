#include "header.h"
#include "bus.h"

START_SCOPE(bus)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* bus)
{
	createWindowClass(L"busWindowClass", windowCallback);
	createChildWindow(L"busWindowClass", parent, bus);
}
void setTrackEvent(HANDLE* eventArray, uint trackCount)
{
	for(uint i = 0; i != trackCount; ++i) 
	{
		SetEvent(eventArray[i]);
	}
}
void startAccumulator(HANDLE accumulatorEvent)
{
	SetEvent(accumulatorEvent);
}
void load(__m256** input, __m256** output, uint inputTrackCount, uint outputTrackCount, uint iterationCount)
{
	__m256* inputAVX2 = *input;
	__m256* outputAVX2 = *output;

	float scalar = 1.0f / (float)inputTrackCount;
	__m256 scalarAVX2 = _mm256_broadcast_ss(&scalar);
	
	for(uint i = 0; i != iterationCount; ++i)
	{
		__m256 sum = {};
		for(uint j = 0; j != inputTrackCount; ++j)
		{
			sum = _mm256_add_ps(sum, *inputAVX2);
			++inputAVX2;
		}
		sum = _mm256_mul_ps(sum, scalarAVX2);
		*outputAVX2 = sum;
		outputAVX2 += outputTrackCount;
	}
	*input = inputAVX2;
	*output = outputAVX2;
}

DWORD WINAPI loader(LPVOID parameter)
{
	State* state = (State*)parameter;

	HANDLE* eventArray = state->inputLoadEventArray;

	uint inputTrackCount = state->inputTrackCount;
	uint outputTrackCount = state->outputTrackCount;
	uint* inputFinishCount = &state->inputFinishCount;
	uint* outputFinishCount = state->outputFinishCount;

	uint outputBufferOffset = state->outputTrackNumber * AVX2_FRAME_SIZE;

	RingBuffer* inputBuffer = &state->inputBuffer;
	RingBuffer* outputBuffer = &state->outputBuffer;
	__m256* input = (__m256*)state->inputBuffer.start;
	__m256* output = (__m256*)state->outputBuffer.start;

	HANDLE outputLoadEvent = state->outputLoadEvent;
	HANDLE outputExitEvent = state->outputExitEvent;
	HANDLE inputExitSemaphore = state->inputExitSemaphore;
	HANDLE inputFinishSemaphore = state->inputFinishSemaphore;
	HANDLE outputFinishSemaphore = state->outputFinishSemaphore;
	HANDLE waitHandle[] = {outputLoadEvent, outputExitEvent};

	uint loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;
	uint running = 1;
	while(running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{
				checkCompletion(inputFinishCount, inputTrackCount, inputFinishSemaphore);
 				load(&input, &output, inputTrackCount, outputTrackCount, iterationCount);

				setTrackEvent(eventArray, inputTrackCount);

				boundCheck(inputBuffer, (void**)&input, 0);
				boundCheck(outputBuffer, (void**)&output, outputBufferOffset);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				checkCompletion(inputFinishCount, inputTrackCount, inputFinishSemaphore);
				ReleaseSemaphore(inputExitSemaphore, (long)inputTrackCount, 0);
				checkCompletion(inputFinishCount, inputTrackCount, inputFinishSemaphore);
				running = 0;
			}
		}
		InterlockedIncrement(outputFinishCount);
		ReleaseSemaphore(outputFinishSemaphore, 1, 0);

	}
    return 0;
}
void createInputBuffer(State* state)
{
	uint frameCount = globalState.audioEndpointFrameCount;
	uint channelCount = 2;
	uint frameSize = sizeof(float) * channelCount;
	uint trackCount = state->inputTrackCount;
	uint64 bufferSize = (uint64)trackCount * frameCount * frameSize;
	char* bufferStart = {};
	allocateSmallMemory(bufferSize, (void**)&bufferStart);

	state->inputBuffer.start = bufferStart;
	state->inputBuffer.end = bufferStart + bufferSize;
}
void startTrackLoader(State* state)
{
	uint trackCount = state->inputTrackCount;
	HWND* inputLoaderArray = state->inputLoaderArray;
	HANDLE* eventArray = state->inputLoadEventArray;
	HANDLE exitSemaphore = state->inputExitSemaphore; 
	HANDLE finishSemaphore = state->inputFinishSemaphore;

	for(uint i = 0; i != trackCount; ++i)
	{
		Loader trackLoader = {};
		trackLoader.buffer = state->inputBuffer;
		trackLoader.loadEvent = eventArray[i];
		trackLoader.exitSemaphore = exitSemaphore;
		trackLoader.finishSemaphore = finishSemaphore;
		trackLoader.finishCount = &state->inputFinishCount;
		trackLoader.trackCount = trackCount;
		trackLoader.trackNumber = i;

		SendMessage(inputLoaderArray[i], WM_STARTLOADER, (WPARAM)&trackLoader, 0);
	}
}
void startLoader(State* state, WPARAM wParam)
{
	freeSmallMemory(state->inputBuffer.start);
	state->inputFinishCount = 0;

	createInputBuffer(state);
	startTrackLoader(state);

	Loader* busLoader = (Loader*)wParam;
	state->outputExitEvent = busLoader->exitSemaphore;
	state->outputLoadEvent = busLoader->loadEvent;
 	state->outputFinishSemaphore = busLoader->finishSemaphore;
	state->outputFinishCount = busLoader->finishCount;
	state->outputBuffer = busLoader->buffer;
	state->outputTrackCount = busLoader->trackCount;
	state->outputTrackNumber = busLoader->trackNumber;
	createThread(loader, state, 0);
}
void assignBus(State* state, WPARAM wParam)
{
	uint trackCount = state->inputTrackCount;
	HWND audioTrack = (HWND)wParam;
	state->inputLoaderArray[trackCount] = audioTrack;

	HANDLE loadEvent;
	createEvent(0, &loadEvent);
	state->inputLoadEventArray[trackCount] = loadEvent;
	++state->inputTrackCount;
}

void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	state->inputTrackCount = 0;
	state->inputFinishCount = 0;
	state->inputBuffer.start = 0;

	createSemaphore(0, 10, &state->inputExitSemaphore);
	createSemaphore(0, 10, &state->inputFinishSemaphore);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch(message)
	{
		case WM_CREATE:
		{
			initialize(window);
			break;
		}
		case WM_STARTLOADER:
		{
			startLoader(state, wParam);
			break;
		}
		case WM_ASSIGNBUS:
		{
			assignBus(state, wParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
