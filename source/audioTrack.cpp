#include "header.h"
#include "audioTrack.h"

START_SCOPE(audioTrack)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->outputLoaderCount = 0;
	state->outputLoaderNumber = 0;

	createSemaphore(0, INT_MAX, &state->exitSemaphore);
	createArray(&state->audioClipArrayHandle, sizeof(AudioClip*));

	createArray(&state->trackControlArrayHandle, sizeof(TrackControl));
	TrackControl trackControl = {};
	trackControl.gain = _mm256_set1_ps(1.0f);
	trackControl.pan = _mm256_set1_ps(1.0f);
	arrayAppend(state->trackControlArrayHandle, &trackControl);
	getArrayStart(state->trackControlArrayHandle, (void**)&state->activeTrackControl);

	createArray(&state->loaderTrackControlArrayHandle, sizeof(TrackControl*));
	createArray(&state->outputLoaderEventArrayHandle, sizeof(HANDLE));
	createArray(&state->outputBusArrayHandle, sizeof(uint));
	uint outputBus = 0;
	arrayAppend(state->outputBusArrayHandle, &outputBus);

    createWindowClass(L"audioTrackWindowClass", windowCallback);
    createChildWindow(L"audioTrackWindowClass", parent, window, state);
}
void addClip(State* state, WPARAM wParam)
{
	void* audioClipArrayHandle = state->audioClipArrayHandle;
	AudioClip* audioClip = (AudioClip*)wParam;
	arrayAppend(audioClipArrayHandle, &audioClip);
}
void chooseClip(AudioClip** clipList, AudioClip** selectedClip, uint clipCount)
{
	uint64 readCursor = globalState.readCursor;
	for(uint i = 0; i != clipCount; ++i)
	{
		uint64 endFrame = clipList[i]->endFrame;
		if (readCursor < endFrame)
		{
			*selectedClip = clipList[i];
			break;
		}
	}
}
void fillSample(float* input, float* output, __m256* scaler, uint iterationCount, uint inputLoaderCount)
{
	__m256* inputAVX2 = (__m256*)input;
	__m256* outputAVX2 = (__m256*)output;
	for(uint i = 0; i != iterationCount; ++i)
	{
		__m256 sample = _mm256_load_ps((float*)inputAVX2);
		sample = _mm256_mul_ps(sample, *scaler);
		_mm256_store_ps((float*)outputAVX2, sample);
		outputAVX2 += inputLoaderCount;
		++inputAVX2;
	}
}
void checkStart(uint64 startFrame, uint64 readCursor, uint* loadCase)
{
	if(readCursor == startFrame)
	{
		*loadCase = 2;
	}
}
void checkClipEnd(uint64 readCursor, uint64 endFrame, uint* loadCase)
{
	if(readCursor == endFrame)
	{
		*loadCase = 3;
	}
}
void fillZero(float* destination, uint iterationCount)
{
	__m256 zero = _mm256_setzero_ps();
	__m256* destinationAVX2 = (__m256*)destination;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, zero);
		++destinationAVX2;
	}
}
void checkFillCount(uint* loadCase, uint zeroFillCount, uint caseToSwitch)
{
	if(zeroFillCount == 0)
	{
		*loadCase = caseToSwitch;
	}
}
void prepareClip(AudioClip* audioClip, float** sample, uint* loadCase)
{
	if(audioClip == 0)
	{
		*loadCase = 0;
		return;
	}

	uint64 readCursor = globalState.readCursor;
	uint64 start = audioClip->startFrame;
	float* sampleChunk = audioClip->waveFile.sampleChunk;
	if(readCursor < start)
	{
		*loadCase = 0;
	}
	else if(readCursor == start)
	{
		*loadCase = 2;
	}
	else
	{
		uint64 offset = readCursor - start;
		uint64 loadFrameCount = globalState.audioEndpointFrameCount;
		uint64 offsetFrameCount = offset * loadFrameCount;
		uint channelCount = audioClip->waveFile.header.channelCount;
		sampleChunk += (channelCount * offsetFrameCount);
		*loadCase = 2;
	}
	*sample = sampleChunk;
}
void selectNextClip(AudioClip** audioClip)
{
	AudioClip* nextClip = *audioClip + 1;
	if(nextClip->startFrame == 0)
	{
		*audioClip = 0;
	}
	else
	{
		audioClip += 1;
	}
}
void copySample(float** input, float* output, uint iterationCount)
{
	__m256* inputAVX2 = (__m256*)*input;
	__m256* outputAVX2 = (__m256*)output;
	for (uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)outputAVX2, *inputAVX2);
		++outputAVX2;
		++inputAVX2;
	}
	*input = (float*)inputAVX2;
}
void process(AudioEffect* audioEffectList, uint effectCount, float* sample, uint iterationCount)
{	
	for(uint i = 0; i != effectCount; ++i)
	{
		ProcessFunction process  = audioEffectList->process;
		void* state = audioEffectList->state;
		process(sample, iterationCount, state);
		++audioEffectList;
	}
}
void processGain(float* sample, uint iterationCount, void* state)
{
	__m256 gain = *((__m256*)state);
	__m256* sampleAVX2 = (__m256*)sample;
	for(uint i = 0; i != iterationCount; ++i)
	{
		*sampleAVX2 = _mm256_mul_ps(gain, *sampleAVX2);
		++sampleAVX2;
	}
}
DWORD WINAPI busLoader(LPVOID parameter)
{
	BusLoaderInfo* busLoaderInfo = (BusLoaderInfo*)parameter;
	BufferInfo* outputBufferInfo = &busLoaderInfo->outputBufferInfo;

	uint bufferOffset = outputBufferInfo->loaderPosition * AVX2_FRAME_SIZE;
	uint loaderCount = outputBufferInfo->loaderCount;
	float* ouputBuffer = (float*)((char*)outputBufferInfo->buffer + bufferOffset);
	float* inputBuffer = busLoaderInfo->inputBuffer;

	HANDLE completeSemaphore = outputBufferInfo->bufferCompleteSemaphore;

	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

	__m256* gain = &busLoaderInfo->trackControl.gain;
	__m256* pan = &busLoaderInfo->trackControl.pan;

	HANDLE startBusLoaderEvent = busLoaderInfo->startBusLoaderEvent;
	HANDLE exitSemaphore = busLoaderInfo->exitSemaphore;
	HANDLE waitHandle[] = {startBusLoaderEvent, exitSemaphore};
	uint running = 1;
	while(running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch (signal)
		{
			case WAIT_OBJECT_0:
			{
				__m256 scaler = _mm256_mul_ps(*gain, *pan);
				fillSample(inputBuffer, ouputBuffer, &scaler, iterationCount, loaderCount);
				ReleaseSemaphore(completeSemaphore, 1, 0);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				running = 0;
				freeSmallMemory(busLoaderInfo);
			}
		}
	}
	return 0;
}
DWORD WINAPI processor(LPVOID parameter)
{
	TrackProcessorInfo* trackProcessorInfo = (TrackProcessorInfo*)parameter;

	AudioClip** clipList = trackProcessorInfo->clipList;
	uint clipCount = trackProcessorInfo->clipCount;
	AudioClip* selectedClip;
	chooseClip(clipList, &selectedClip, clipCount);

	float* inputBuffer = trackProcessorInfo->inputBuffer;
	float* sample = {};
	uint loadCase = {};
	prepareClip(selectedClip, &sample, &loadCase);

	uint loaderCount = trackProcessorInfo->outputLoaderCount;
	uint64 startFrame = {};
	uint64 endFrame = {};
	if(selectedClip)
	{
		startFrame = selectedClip->startFrame;
		endFrame = selectedClip->endFrame - 1;
	}

	uint64 readCursor = globalState.readCursor + 1;
	uint loadFrameCount = globalState.audioEndpointFrameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

	HANDLE loadEvent = trackProcessorInfo->startTrackProcessorEvent;
	HANDLE exitSemaphore = trackProcessorInfo->exitSemaphore;
	HANDLE* outputLoaderStartEventArray = trackProcessorInfo->busLoaderStartEventArray;
	HANDLE waitHandle[] = {loadEvent, exitSemaphore};

	uint running = 1;
	while(running)
	{
		++readCursor;
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{	
				switch(loadCase)
				{
					case 0:
					{
						fillZero(inputBuffer, iterationCount);
						loadCase = 1;
					} 
					case 1:
					{
						checkStart(startFrame, readCursor, &loadCase);
						break;
					}
					case 2:
					{	
						copySample(&sample, inputBuffer, iterationCount);
						//process(audioEffectList, effectCount, sample, iterationCount);
						checkClipEnd(readCursor, endFrame, &loadCase);
						break;
					}
					case 3:
					{
						selectNextClip(&selectedClip);
						prepareClip(selectedClip, &sample, &loadCase);
					}
					case 4:
					{
					}
				}
				setEventArray(outputLoaderStartEventArray, loaderCount);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				freeSmallMemory(trackProcessorInfo);
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
	busLoaderInfo->exitSemaphore = state->exitSemaphore;
	busLoaderInfo->inputBuffer = state->inputBuffer;
	busLoaderInfo->trackControl = *state->activeTrackControl;

	void* loaderTrackControlArrayHandle = state->loaderTrackControlArrayHandle;
	TrackControl* trackControl = &busLoaderInfo->trackControl;
	state->activeTrackControl = trackControl;
	arrayAppend(loaderTrackControlArrayHandle, &trackControl);

	void* outputLoaderEventArrayHandle = state->outputLoaderEventArrayHandle;
	HANDLE* outputLoaderEventArray = {};
	getArrayStart(outputLoaderEventArrayHandle, (void**)&outputLoaderEventArray);
	uint outputLoaderNumber = state->outputLoaderNumber;
	busLoaderInfo->startBusLoaderEvent = outputLoaderEventArray[outputLoaderNumber];
	++state->outputLoaderNumber;

    createThread(busLoader, busLoaderInfo, 0);
}
void sendControl(State* state, HWND window)
{
	HWND parent = GetAncestor(window, GA_PARENT);
	TrackControl* trackControl = state->activeTrackControl;
	SendMessage(parent, WM_SENDCONTROL, (WPARAM)window, (LPARAM)trackControl);
	state->controlSet = 1;
}
void startProcessor(State* state, WPARAM wParam)
{
    TrackProcessorInfo* trackProcessorInfo = {};
    allocateSmallMemory(sizeof(TrackProcessorInfo), (void**)&trackProcessorInfo);
    
	void* audioClipArrayHandle = state->audioClipArrayHandle;
	AudioClip** audioClipArray = {};
	uint clipCount = {};
	getArray(audioClipArrayHandle, (void**)&audioClipArray, &clipCount);

    trackProcessorInfo->clipList = audioClipArray;
    trackProcessorInfo->clipCount = clipCount;
    trackProcessorInfo->inputBuffer = state->inputBuffer;

	void* outputLoaderEventArrayHandle = state->outputLoaderEventArrayHandle;
	HANDLE* outputLoaderEventArray = {};
	uint outputLoaderCount = {};
	getArray(outputLoaderEventArrayHandle, (void**)&outputLoaderEventArray, &outputLoaderCount);
    trackProcessorInfo->busLoaderStartEventArray = outputLoaderEventArray;
    trackProcessorInfo->outputLoaderCount = outputLoaderCount;

    trackProcessorInfo->startTrackProcessorEvent = (HANDLE)wParam;
    trackProcessorInfo->exitSemaphore = state->exitSemaphore;

	createThread(processor, (LPVOID)trackProcessorInfo, 0);
}
void sendOutputLoader(State* state, HWND window)
{
	void* outputBusArrayHandle = state->outputBusArrayHandle;
	uint* outputBusArray = {};
	uint outputBusCount = {};
	getArray(outputBusArrayHandle, (void**)&outputBusArray, &outputBusCount);

	HWND parent = GetAncestor(window, GA_PARENT);
	for(uint i = 0; i != outputBusCount; ++i)
	{
		SendMessage(parent, WM_SETINPUTLOADER, (WPARAM)window, outputBusArray[i]);
	}
}
void stopProcessor(State* state)
{
    uint outputLoaderNumber = state->outputLoaderNumber;
    if(outputLoaderNumber)
    {
		TrackControl* trackControl = {};
		void* trackControlArrayHandle = state->trackControlArrayHandle;
		getArrayStart(trackControlArrayHandle, (void**)&trackControl);
		*trackControl = *state->activeTrackControl;
		state->activeTrackControl = trackControl;
        HANDLE exitSemaphore = state->exitSemaphore;
        ReleaseSemaphore(exitSemaphore, (long)(outputLoaderNumber + 1), 0);
        waitForSemaphore(exitSemaphore);

		state->outputLoaderNumber = 0;
		state->outputLoaderCount = 0;
		freeSmallMemory(state->inputBuffer);
    }
}
void createBuffer(State* state, WPARAM wParam)
{
	if(state->outputLoaderCount == 0)
	{
		return;
	}
	uint frameCount = globalState.audioEndpointFrameCount;
	uint channelCount = 2;
	uint frameSize = sizeof(float) * channelCount;
	uint bufferSize = frameSize * frameCount;

	float* inputBuffer = {};
	allocateSmallMemory(bufferSize, (void**)&inputBuffer);
	state->inputBuffer = inputBuffer;

	startProcessor(state, wParam);
}
void setOutputLoader(State* state)
{
	++state->outputLoaderCount;
	uint outputLoaderCount = state->outputLoaderCount;
	void* outputLoaderEventArrayHandle = state->outputLoaderEventArrayHandle;
	uint elementCount = {};
	getElementCount(outputLoaderEventArrayHandle, &elementCount);
	if(elementCount < outputLoaderCount)
	{
		uint newElementCount = outputLoaderCount - elementCount;
		for(uint i = 0; i != newElementCount; ++i)
		{
			HANDLE startEvent;
			createEvent(0, &startEvent);
			arrayAppend(outputLoaderEventArrayHandle, &startEvent);
		}
	}
}
void setControl(State* state, WPARAM wParam)
{
	TrackControl* newTrackControl = (TrackControl*)wParam;
	TrackControl* oldTrackControl = state->activeTrackControl;
	*oldTrackControl = *newTrackControl;
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
		case WM_FILEDROP:
		{
			addClip(state, wParam);
			break;
		}
		case WM_STARTOUTPUTLOADER:
		{
			startOutputLoader(state, wParam);
			break;
		}
        case WM_PAUSE:
        {
            stopProcessor(state);
            break;
        }
		case WM_SENDOUTPUTLOADER:
		{
			sendOutputLoader(state, window);
			break;
		}
		case WM_SETOUTPUTLOADER:
		{
			setOutputLoader(state);
			break;
		}
		case WM_CREATEBUFFER:
		{
			createBuffer(state, wParam);
			break;
		}
		case WM_SENDCONTROL:
		{
			sendControl(state, window);
			break;
		}
		case WM_SETCONTROL:
		{
			setControl(state, wParam);
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
