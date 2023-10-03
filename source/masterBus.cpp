#include "header.h"
#include "masterBus.h"

START_SCOPE(masterBus)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* bus, HWND wasapi)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
    state->wasapi = wasapi;
	state->outputLoaderCount = 0;
	state->outputLoaderNumber = 0;
	state->inputBuffer = 0;
    state->outputSet = 0;
    state->inputSet = 0;
	state->inputBufferCompleteCount = 0;

	createArray(&state->inputLoaderArrayHandle, sizeof(HWND));
	createEvent(0, &state->startBusLoaderEvent);
	createSemaphore(0, 10, &state->exitSemaphore);
	createSemaphore(0, 10, &state->bufferCompleteSemaphore);

	createWindowClass(L"masterBusWindowClass", windowCallback);
	createChildWindow(L"masterBusWindowClass", parent, bus, state);
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
void fillSample(float* input, float* output, uint iterationCount, uint inputLoaderCount)
{
	__m256* sourceAVX2 = (__m256*)input;
	__m256* destinationAVX2 = (__m256*)output;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, *sourceAVX2);
		destinationAVX2 += inputLoaderCount;
		++sourceAVX2;
	}
}
DWORD WINAPI masterBusLoader(LPVOID parameter)
{
    MasterBusLoaderInfo* masterBusLoaderInfo = (MasterBusLoaderInfo*)parameter;

	float* inputBuffer = masterBusLoaderInfo->inputBuffer;
	BufferInfo* bufferInfo = &masterBusLoaderInfo->outputBufferInfo;
    float* outputBuffer = bufferInfo->buffer;
	HANDLE completeSemaphore = bufferInfo->bufferCompleteSemaphore;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

    HANDLE loadOutputEvent = masterBusLoaderInfo->loadOutputEvent;
	SetEvent(loadOutputEvent);

    HANDLE startBusLoaderEvent = masterBusLoaderInfo->startBusLoaderEvent;
    HANDLE exitSemaphore = masterBusLoaderInfo->exitSemaphore;
    HANDLE waitHandle[] = { startBusLoaderEvent, exitSemaphore};

    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
                WaitForSingleObject(loadOutputEvent, INFINITE);
                fillSample(inputBuffer, outputBuffer, iterationCount, 1);
				ReleaseSemaphore(completeSemaphore, 1, 0);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeSmallMemory(masterBusLoaderInfo);
                running = 0;
            }
        }
    }
    return 0;
}
void process(float* buffer, uint iterationCount, uint inputLoaderCount)
{
	float scalar = 1.0f / (float)inputLoaderCount;
	__m256 scalarAVX2 = _mm256_broadcast_ss(&scalar);
	
    __m256* trackSampleInput = (__m256*)buffer;
    __m256* trackSampleOutput = (__m256*)buffer;
	for(uint i = 0; i != iterationCount; ++i)
	{
		__m256 sum = {};
		for(uint j = 0; j != inputLoaderCount; ++j)
		{
			sum = _mm256_add_ps(sum, *trackSampleInput);
			++trackSampleInput;
		}
		sum = _mm256_mul_ps(sum, scalarAVX2);
		*trackSampleOutput = _mm256_load_ps((float*)&sum);
        ++trackSampleOutput;
	}
}
DWORD WINAPI busProcessor(LPVOID parameter)
{
    BusProcessorInfo* busProcessorInfo = (BusProcessorInfo*)parameter;
    uint inputLoaderCount = busProcessorInfo->inputLoaderCount;
    float* inputBuffer = busProcessorInfo->inputBuffer;
    HANDLE* busLoaderStartEventArray = busProcessorInfo->busLoaderStartEventArray;
	uint outputLoaderCount = busProcessorInfo->outputLoaderCount;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

    uint running = 1;
    HANDLE completeSemaphore = busProcessorInfo->bufferCompleteSemaphore;
    HANDLE exitSemaphore = busProcessorInfo->exitSemaphore;
    HANDLE waitHandle[] = { completeSemaphore, exitSemaphore};
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
				checkCompletion(completeSemaphore, inputLoaderCount);
                process(inputBuffer, iterationCount, inputLoaderCount);
                setEventArray(busLoaderStartEventArray, outputLoaderCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeSmallMemory(busProcessorInfo);
                running = 0;
            }
        }
    }
	return 0;
}
void startOutputLoader(State* state, WPARAM wParam, LPARAM lParam)
{
	BufferInfo* outputBufferInfo = (BufferInfo*)wParam;
    HANDLE loadOutputEvent = (HANDLE)lParam;

    MasterBusLoaderInfo* masterBusLoaderInfo = {};
    allocateSmallMemory(sizeof(MasterBusLoaderInfo), (void**)&masterBusLoaderInfo);
    masterBusLoaderInfo->outputBufferInfo = *outputBufferInfo;
    masterBusLoaderInfo->inputBuffer = state->inputBuffer;
    masterBusLoaderInfo->exitSemaphore = state->exitSemaphore;

	uint outputLoaderNumber = state->outputLoaderNumber;
	masterBusLoaderInfo->startBusLoaderEvent = state->startBusLoaderEvent;
	
    masterBusLoaderInfo->loadOutputEvent = loadOutputEvent; 

    createThread(masterBusLoader, masterBusLoaderInfo, 0);
}
void createProcessor(State* state)
{
    BusProcessorInfo* busProcessorInfo = {};
    allocateSmallMemory(sizeof(BusProcessorInfo), (void**)&busProcessorInfo);
    
    busProcessorInfo->inputBuffer = state->inputBuffer;
    busProcessorInfo->busLoaderStartEventArray = &state->startBusLoaderEvent;
    busProcessorInfo->outputLoaderCount = state->outputLoaderCount;

	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
	uint inputLoaderCount = {};
	getElementCount(inputLoaderArrayHandle, &inputLoaderCount);
    busProcessorInfo->inputLoaderCount = inputLoaderCount;
    busProcessorInfo->bufferCompleteSemaphore = state->bufferCompleteSemaphore;
    busProcessorInfo->exitSemaphore = state->exitSemaphore;

	createThread(busProcessor, (LPVOID)busProcessorInfo, 0);
}
void startInputLoader(State* state)
{
	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
    HWND* inputLoaderArray = {};
    uint inputLoaderCount = {};
	getArray(inputLoaderArrayHandle, (void**)&inputLoaderArray, &inputLoaderCount);

	BufferInfo outputBufferInfo = {};
    outputBufferInfo.buffer = state->inputBuffer;
    outputBufferInfo.bufferCompleteSemaphore = state->bufferCompleteSemaphore;
    outputBufferInfo.loaderCount = inputLoaderCount;

    for(uint i = 0; i != inputLoaderCount; ++i)
    {
        outputBufferInfo.loaderPosition = i;
        SendMessage(inputLoaderArray[i], WM_STARTOUTPUTLOADER, (WPARAM)&outputBufferInfo, 0);
    }
}
void sendOutputLoader(State* state, HWND window)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_SETINPUTLOADER, (WPARAM)window, 0);
}
void setInputLoader(State* state, HWND window, WPARAM wParam)
{
	if (state->outputSet == 0)
	{
		sendOutputLoader(state, window);
		state->outputSet = 1;
	}
	HWND inputLoader = (HWND)wParam;
	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
	arrayAppend(inputLoaderArrayHandle, &inputLoader);
}
void stopProcessor(State* state)
{
	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
	uint inputLoaderCount = {};
	getElementCount(inputLoaderArrayHandle, &inputLoaderCount);
    if(inputLoaderCount != 0)
    {
        HANDLE exitSemaphore = state->exitSemaphore;
        ReleaseSemaphore(exitSemaphore, (long)(1 + 1), 0);
        waitForSemaphore(exitSemaphore);
		state->outputLoaderNumber = 0;
		state->outputLoaderCount = 0;
		state->outputSet = 0;
		state->inputSet = 0;
		freeSmallMemory(state->inputBuffer);
		resetArray(inputLoaderArrayHandle);
    }
}
void createBuffer(State* state)
{
	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
	uint inputLoaderCount;
	getElementCount(inputLoaderArrayHandle, &inputLoaderCount);
	if (inputLoaderCount == 0)
	{
		return;
	}

	uint frameCount = globalState.audioEndpointFrameCount;
	uint channelCount = 2;
	uint frameSize = sizeof(float) * channelCount;
	uint loaderCount = inputLoaderCount;
	uint64 bufferSize = (uint64)loaderCount * frameCount * frameSize;

	float* bufferStart = {};
	allocateSmallMemory(bufferSize, (void**)&bufferStart);

	state->inputBuffer = bufferStart;
	createProcessor(state);
}
void sendInputLoader(State* state)
{
	void* inputLoaderArrayHandle = state->inputLoaderArrayHandle;
	HWND* inputLoaderArray = {};
	uint inputLoaderCount = {};
	getArray(inputLoaderArrayHandle, (void**)&inputLoaderArray, &inputLoaderCount);
	for (uint i = 0; i != inputLoaderCount; ++i)
	{
		SendMessage(inputLoaderArray[i], WM_SETOUTPUTLOADER, 0, 0);
	}
}
void setOutputLoader(State* state)
{
	if (state->inputSet == 0)
	{
		sendInputLoader(state);
		state->inputSet = 1;
	}
	++state->outputLoaderCount;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch(message)
	{
		case WM_CREATE:
		{
			setState(window, lParam);
			break;
		}
        case WM_STARTOUTPUTLOADER:
        {
            startOutputLoader(state, wParam, lParam);
            break;
        }
        case WM_STARTINPUTLOADER:
        {
            startInputLoader(state);
            break;
        }
		case WM_SETINPUTLOADER:
		{
			setInputLoader(state, window, wParam);
			break;
		}
		case WM_SETOUTPUTLOADER:
		{
			setOutputLoader(state);
			break;
		}
		case WM_CREATEBUFFER:
		{
			createBuffer(state);
			break;
		}
        case WM_PAUSE:
        {
            stopProcessor(state);
            break;
        }
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
