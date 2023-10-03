#include "header.h"
#include "bus.h"

START_SCOPE(bus)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* bus)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->inputLoaderCount = 0;
	state->inputBufferCompleteCount = 0;
	state->outputLoaderCount = 0;
	state->outputLoaderNumber = 0;
    state->outputBusArray[0] = 0;
    state->outputBusCount = 1;
	state->inputBuffer = 0;
    state->outputSet = 0;
	state->inputSet = 0;

	createSemaphore(0, 10, &state->exitSemaphore);
	createSemaphore(0, 10, &state->bufferCompleteSemaphore);
	createEvent(0, &state->busLoaderStartEventArray[0]);

	createWindowClass(L"busWindowClass", windowCallback);
	createChildWindow(L"busWindowClass", parent, bus, state);
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
	__m256* inputAVX2 = (__m256*)input;
	__m256* outputAVX2 = (__m256*)output;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)outputAVX2, *inputAVX2);
		outputAVX2 += inputLoaderCount;
		++inputAVX2;
	}
}
DWORD WINAPI busLoader(LPVOID parameter)
{
    BusLoaderInfo* busLoaderInfo = (BusLoaderInfo*)parameter;

	BufferInfo* outputBufferInfo = &busLoaderInfo->outputBufferInfo;
    uint bufferOffset = outputBufferInfo->loaderPosition * AVX2_FRAME_SIZE;
	uint loaderCount = outputBufferInfo->loaderCount;
	float* outputBuffer = (float*)((char*)outputBufferInfo->buffer + bufferOffset);

	float* inputBuffer = busLoaderInfo->inputBuffer;
	HANDLE completeSemaphore = outputBufferInfo->bufferCompleteSemaphore;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

    HANDLE startBusLoaderEvent = busLoaderInfo->startBusLoaderEvent;
    HANDLE exitSemaphore = busLoaderInfo->exitSemaphore;
    HANDLE waitHandle[] = { startBusLoaderEvent, exitSemaphore};
    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
				fillSample(inputBuffer, outputBuffer, iterationCount, loaderCount);
				ReleaseSemaphore(completeSemaphore, 1, 0);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeSmallMemory(busLoaderInfo);
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
	uint outputLoaderCount = busProcessorInfo->outputLoaderCount;
    float* inputBuffer = busProcessorInfo->inputBuffer;
    HANDLE* busLoaderStartEventArray = busProcessorInfo->busLoaderStartEventArray;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

    HANDLE completeSemaphore = busProcessorInfo->bufferCompleteSemaphore;
    HANDLE exitSemaphore = busProcessorInfo->exitSemaphore;
    HANDLE waitHandle[] = { completeSemaphore, exitSemaphore};

    uint running = 1;
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
void startOutputLoader(State* state, WPARAM wParam)
{
	BufferInfo* bufferInfo = (BufferInfo*)wParam;

    BusLoaderInfo* busLoaderInfo = {};
    allocateSmallMemory(sizeof(BusLoaderInfo), (void**)&busLoaderInfo);
    busLoaderInfo->outputBufferInfo = *bufferInfo;
    busLoaderInfo->inputBuffer = state->inputBuffer;
    busLoaderInfo->exitSemaphore = state->exitSemaphore;

    uint outputLoaderNumber = state->outputLoaderNumber;
    busLoaderInfo->startBusLoaderEvent = state->busLoaderStartEventArray[outputLoaderNumber];
	++state->outputLoaderNumber;

    createThread(busLoader, busLoaderInfo, 0);
}
void createProcessor(State* state)
{
    BusProcessorInfo* busProcessorInfo = {};
    allocateSmallMemory(sizeof(BusProcessorInfo), (void**)&busProcessorInfo);
    
    busProcessorInfo->inputBuffer = state->inputBuffer;
    busProcessorInfo->busLoaderStartEventArray = state->busLoaderStartEventArray;
    busProcessorInfo->outputLoaderCount = state->outputLoaderCount;
    busProcessorInfo->inputLoaderCount = state->inputLoaderCount;
    busProcessorInfo->bufferCompleteSemaphore = state->bufferCompleteSemaphore;
    busProcessorInfo->exitSemaphore = state->exitSemaphore;

	createThread(busProcessor, (LPVOID)busProcessorInfo, 0);
}
void startInputLoader(State* state)
{
    if(state->outputBusCount == 0 || state->inputLoaderCount == 0)
    {
        return;
    }

	BufferInfo bufferInfo = {};
	bufferInfo.buffer = state->inputBuffer;
	bufferInfo.bufferCompleteSemaphore = state->bufferCompleteSemaphore;
	bufferInfo.loaderCount = state->inputLoaderCount;

    HWND* inputLoaderArray = state->inputLoaderArray;
    uint inputLoaderCount = state->inputLoaderCount;
    for(uint i = 0; i != inputLoaderCount; ++i)
    {
		bufferInfo.loaderPosition = i;
        SendMessage(inputLoaderArray[i], WM_STARTOUTPUTLOADER, (WPARAM)&bufferInfo, 0);
    }
}
void stopProcessor(State* state)
{
    uint outputLoaderNumber = state->outputLoaderNumber;
	if(outputLoaderNumber != 0)
    {
		HANDLE exitSemaphore = state->exitSemaphore;
		ReleaseSemaphore(exitSemaphore, (long)(outputLoaderNumber + 1), 0);
		waitForSemaphore(exitSemaphore);
		state->outputLoaderNumber = 0;
		state->outputLoaderCount = 0;
		state->inputLoaderCount = 0;
		state->outputSet = 0;
		state->inputSet = 0;
		freeSmallMemory(state->inputBuffer);
    }
}
void sendOutputLoader(State* state, HWND window)
{
	uint* outputBusArray = state->outputBusArray;
	uint outputBusCount = state->outputBusCount;
	HWND parent = GetAncestor(window, GA_PARENT);
	for (uint i = 0; i != outputBusCount; ++i)
	{
		SendMessage(parent, WM_SETINPUTLOADER, (WPARAM)window, outputBusArray[i]);
	}
}
void setInputLoader(State* state, HWND window, WPARAM wParam)
{
	if(state->outputSet == 0)
	{
		sendOutputLoader(state, window);
		state->outputSet = 1;
	}
	uint inputLoaderCount = state->inputLoaderCount;
	state->inputLoaderArray[inputLoaderCount] = (HWND)wParam;
	++state->inputLoaderCount;
}
void createBuffer(State* state)
{
	if(state->inputLoaderCount == 0 || state->outputLoaderCount == 0)
	{
		return;
	}


	uint frameCount = globalState.audioEndpointFrameCount;
	uint channelCount = 2;
	uint frameSize = sizeof(float) * channelCount;
	uint loaderCount = state->inputLoaderCount;
	uint64 bufferSize = (uint64)loaderCount * frameCount * frameSize;

	float* bufferStart = {};
	allocateSmallMemory(bufferSize, (void**)&bufferStart);

	state->inputBuffer = bufferStart;

	createProcessor(state);
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
void setOutputLoader(State* state)
{
	if(state->inputSet == 0)
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
            startOutputLoader(state, wParam);
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
	    case WM_SETINPUT:
		{
			//setInput(state, window, wParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
