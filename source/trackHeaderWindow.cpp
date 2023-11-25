#include "include.hpp"
#include "trackHeaderWindow.hpp"

START_SCOPE(trackHeaderWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(String* trackName, HWND parent, RECT* boundingBox, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->outputSet = 0;
    state->inputSet = 0;
    state->outputLoaderNumber = 0;
    state->audioClipArrayHandle = 0;

    createWindowClass(L"trackHeaderWindowClass", windowCallback);
    createChildWindow(L"trackHeaderWindowClass", parent, state, window);
    
    createDynamicArray(&state->inputTrackArrayHandle, sizeof(HWND));
    createDynamicArray(&state->outputBusNumberArrayHandle, sizeof(uint));
    uint busNumber = 0;
    appendElement(state->outputBusNumberArrayHandle, (char*)&busNumber);
    createDynamicArray(&state->startEventArrayHandle, sizeof(HANDLE));

    createSemaphore(&state->finishSemaphore);
    createSemaphore(&state->exitSemaphore);

    int x = boundingBox->left;
    int y = boundingBox->top;
    int width = boundingBox->right;
    int height = boundingBox->bottom;
    placeWindow(*window, x, y, width, height);
    SendMessage(*window, WM_CREATECHILD, (WPARAM)trackName, 0);
}
void paint(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

    int width, height;
    getWindowDimension(window, &width, &height);

	RECT invalidRectangle = {0, 0, width, height}; 
	rectangleFill(deviceContext, &invalidRectangle, COLOR_BLACK);
	rectangleFrame(deviceContext, &invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void createChild(State* state, HWND window, WPARAM wParam)
{
    String* text = (String*)wParam;
    int width, height;
    getWindowDimension(window, &width, &height);
    height = 16;

    RECT boundingBox = {0, 0, width, height};
    HWND textbox = {};
    textboxWindow::create(text, &boundingBox, window, &textbox);
    state->textbox = textbox;
}
void setOutput(State* state, HWND window)
{
    void* outputBusNumberArrayHandle = state->outputBusNumberArrayHandle;
    uint* outputBusNumberArray = {};
    uint outputBusCount = {};
    getArray(outputBusNumberArrayHandle, (void**)&outputBusNumberArray, &outputBusCount);
    HWND parent = GetAncestor(window, GA_PARENT);
    void* startEventArrayHandle = state->startEventArrayHandle;
    uint eventCount = {};
    getOccupiedSlotCount(startEventArrayHandle, &eventCount);
    if(outputBusCount > eventCount)
    {
        uint newEventCount = outputBusCount - eventCount;
        resizeArray(startEventArrayHandle, newEventCount);
        for(uint i = 0; i != newEventCount; ++i)
        {
            HANDLE event = {};
            createEvent(&event);
            appendElement(startEventArrayHandle, &event);
        }
    }
    for(uint i = 0; i != outputBusCount; ++i)
    {
        SendMessage(parent, WM_SETOUTPUT, (WPARAM)window, (LPARAM)outputBusNumberArray[i]);
    }
}
void chooseClip(AudioClip** clipList, uint clipCount, uint* clipNumber, AudioClip* selectedClip, uint64 readCursor)
{
    for(uint i = *clipNumber; i != clipCount; ++i)
    {
        if(readCursor < clipList[i]->endFrame)
        {
            *selectedClip = *(clipList[i]);
            *clipNumber = i + 1;
            break;
        }
    }
}
void prepareToPlay(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
    void* audioClipArrayHandle = (void*)wParam;
    AudioClip** audioClipArray = {};
    uint audioClipCount = {};
    getArray(audioClipArrayHandle, (void**)&audioClipArray, &audioClipCount);

    AudioClip selectedClip = {};
    uint64* tuple = (uint64*)lParam;
    uint64 readCursor = tuple[0];
    uint clipNumber = {};
    chooseClip(audioClipArray, audioClipCount, &clipNumber, &selectedClip, readCursor);
    if(selectedClip.frameCount)
    {
        uint frameCount = (uint)tuple[1];
        uint bufferSize = frameCount * sizeof(float*) * 2;

        float* buffer;
        allocateMemory(bufferSize, (void**)&buffer);
        state->buffer = buffer;

        state->audioClipArrayHandle = audioClipArrayHandle;
        setOutput(state, window);
    }
}
void setInput(State* state, HWND window, WPARAM wParam)
{
    if(!state->outputSet)
    {
        setOutput(state, window);
        state->outputSet = 1;
    }
    HWND inputTrack = (HWND)wParam;
    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    appendElement(inputTrackArrayHandle, &inputTrack);

    ++state->inputLoaderCount;
}
void startInputLoader(State* state, uint frameCount)
{
    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    HWND* trackArray = {};
    uint trackCount = {};
    getArray(inputTrackArrayHandle, (void**)&trackArray, &trackCount);

    float* buffer = {};
    uint bufferSize = frameCount * trackCount * sizeof(float) * 2;
    allocateMemory(bufferSize, (void**)&buffer);
    state->buffer = buffer;

    BufferLoaderInfo loaderInfo = {};
    loaderInfo.outputBuffer = state->buffer;
    loaderInfo.finishSemaphore = state->finishSemaphore;
    loaderInfo.loaderCount = trackCount;
    loaderInfo.frameCount = frameCount;

    startBufferLoader(trackArray, trackCount, &loaderInfo);
}
void startOutputLoader(State* state, WPARAM wParam)
{
    BufferLoaderInfo* bufferLoaderInfo = (BufferLoaderInfo*)wParam;
    uint frameCount = bufferLoaderInfo->frameCount;
    if(!state->inputSet && !state->audioClipArrayHandle)
    {
        startInputLoader(state, frameCount);
        state->inputSet = 1;
    }
    void* startEventArrayHandle = state->startEventArrayHandle;
    HANDLE* startEventArray = {};
    getArrayStart(startEventArrayHandle, (void**)&startEventArray);

    uint outputLoaderNumber = state->outputLoaderNumber;
    bufferLoaderInfo->exitSemaphore = state->exitSemaphore;
    bufferLoaderInfo->inputBuffer = state->buffer;
    bufferLoaderInfo->startEvent = startEventArray[outputLoaderNumber];

    createThread(bufferLoader, bufferLoaderInfo);
    ++state->outputLoaderNumber;
}
DWORD WINAPI busProcessor(LPVOID parameter)
{
    BusProcessorInfo* busProcessorInfo = (BusProcessorInfo*)parameter;
    uint inputLoaderCount = busProcessorInfo->inputLoaderCount;
	uint outputLoaderCount = busProcessorInfo->outputLoaderCount;
    float* inputBuffer = busProcessorInfo->buffer;
    HANDLE* loaderStartEventArray = busProcessorInfo->loaderStartEventArray;

	uint frameCount = busProcessorInfo->frameCount;
    uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;

    HANDLE finishSemaphore = busProcessorInfo->finishSemaphore;
    HANDLE exitSemaphore = busProcessorInfo->exitSemaphore;
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
                accumulateFrame(inputBuffer, inputBuffer, iterationCount, inputLoaderCount);
                setEventArray(loaderStartEventArray, outputLoaderCount);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeMemory(busProcessorInfo);
                running = 0;
            }
        }
    }
	return 0;
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
void prepareClip(AudioClip* audioClip, uint64 readCursor, uint frameCount, float** sample, uint* loadCase)
{
	if(audioClip->frameCount == 0)
	{
		*loadCase = 0;
		return;
	}

	uint64 start = audioClip->startFrame;
	uint64 endFrame = audioClip->startFrame;
	float* sampleChunk = audioClip->sampleChunk;
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
		uint64 offsetFrameCount = offset * frameCount;
		uint channelCount = 2;
		sampleChunk += (channelCount * offsetFrameCount);
		*loadCase = 2;
	}
	*sample = sampleChunk;
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
void fillEndFrame(float** input, float* output, uint endOffsetFrameCount, uint frameCount)
{
	__m256* outputAVX2 = (__m256*)output;
	__m256* inputAVX2 = (__m256*)*input;
    uint framesPerAVX2 = 8;
    uint iterationCount = endOffsetFrameCount / framesPerAVX2;
	for(uint i = 0; i != iterationCount; ++i)
	{
        outputAVX2[i] = inputAVX2[i];
	}
    uint offsetFrameCount = endOffsetFrameCount % framesPerAVX2;
	float* inputFrame = (float*)inputAVX2;
    float* outputFrame = (float*)outputAVX2;
    for(uint i = 0; i != offsetFrameCount; ++i)
    {
        *outputFrame = *inputFrame;
        ++outputFrame;
        ++input;
        *outputFrame = *inputFrame;
        ++outputFrame;
        ++input;

    }
    offsetFrameCount = framesPerAVX2 - offsetFrameCount;
    for(uint i = 0; i != offsetFrameCount; ++i)
    {
        *outputFrame = 0;
        ++outputFrame;
    }
	outputAVX2 = (__m256*)outputFrame;
    iterationCount = (frameCount - endOffsetFrameCount) / framesPerAVX2;
	for(uint i = 0; i != iterationCount; ++i)
	{
        outputAVX2[i] = {};
	}

    *input = (float*)inputFrame;
}
void fillStartFrame(float** input, float* output, uint startOffsetFrameCount)
{
	float* inputFrame = *input;
    float* outputFrame = (float*)((char*)output + startOffsetFrameCount * sizeof(float) * 2);
    uint framesPerAVX2 = 8;
    uint offsetFrameCount = framesPerAVX2 - (startOffsetFrameCount % framesPerAVX2);
    for(uint i = 0; i != offsetFrameCount; ++i)
    {
        *outputFrame = *inputFrame;
        ++outputFrame;
        ++input;
        *outputFrame = *inputFrame;
        ++outputFrame;
        ++input;

    }
	__m256* outputAVX2 = (__m256*)outputFrame;
	__m256* inputAVX2 = (__m256*)inputFrame;
    uint iterationCount = startOffsetFrameCount / framesPerAVX2;
	for(uint i = 0; i != iterationCount; ++i)
	{
        outputAVX2[i] = inputAVX2[i];
	}
    *input = (float*)inputAVX2;
}
DWORD WINAPI trackProcessor(LPVOID parameter)
{
	TrackProcessorInfo* trackProcessorInfo = (TrackProcessorInfo*)parameter;

	void* audioClipArrayHandle = trackProcessorInfo->audioClipArrayHandle;
    AudioClip** clipList = {};
    uint clipCount = {};
    getArray(audioClipArrayHandle, (void**)&clipList, &clipCount);
    AudioClip selectedClip = {};
	uint64 readCursor = trackProcessorInfo->readCursor;
    uint clipNumber = {};
    chooseClip(clipList, clipCount, &clipNumber, &selectedClip, readCursor);

	float* inputBuffer = trackProcessorInfo->buffer;
	uint loaderCount = trackProcessorInfo->outputLoaderCount;

	uint frameCount = trackProcessorInfo->frameCount;
    uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;

	float* sample = {};
	uint loadCase = {};
	prepareClip(&selectedClip, readCursor, frameCount, &sample, &loadCase);
    readCursor /= frameCount;
	uint64 startFrame = selectedClip.startFrame / frameCount;
	uint64 endFrame = (selectedClip.endFrame / frameCount) - 1;

	HANDLE startEvent = trackProcessorInfo->startEvent;
	HANDLE exitSemaphore = trackProcessorInfo->exitSemaphore;
	HANDLE* loaderStartEventArray = trackProcessorInfo->loaderStartEventArray;
	HANDLE waitHandle[] = {startEvent, exitSemaphore};

	uint running = 1;
	while(running)
	{
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
                        break;
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
                        selectedClip = {};
                        chooseClip(clipList, clipCount, &clipNumber, &selectedClip, readCursor);
                        prepareClip(&selectedClip, readCursor, frameCount, &sample, &loadCase);
                        startFrame = selectedClip.startFrame / frameCount;
                        endFrame = (selectedClip.endFrame / frameCount) - 1;
                        break;
                    }
				}
				setEventArray(loaderStartEventArray, loaderCount);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				freeMemory(trackProcessorInfo);
				running = 0;
			}
		}
		++readCursor;
	}
	return 0;
}
void startProcessor(State* state, WPARAM wParam, LPARAM lParam)
{
    uint64* tuple = (uint64*)lParam;
    if(state->audioClipArrayHandle)
    {
        TrackProcessorInfo* trackProcessorInfo = {};
        allocateMemory(sizeof(TrackProcessorInfo), (void**)&trackProcessorInfo);
        trackProcessorInfo->startEvent = (HANDLE)wParam;
        trackProcessorInfo->exitSemaphore = state->exitSemaphore;
        trackProcessorInfo->audioClipArrayHandle = state->audioClipArrayHandle;
        trackProcessorInfo->buffer = state->buffer;

        void* startEventArrayHandle = state->startEventArrayHandle;
        HANDLE* startEventArray = {};
        uint loaderCount = {};
        getArray(startEventArrayHandle, (void**)&startEventArray, &loaderCount);
        trackProcessorInfo->loaderStartEventArray = startEventArray;
        trackProcessorInfo->outputLoaderCount = loaderCount;
        trackProcessorInfo->frameCount = (uint)tuple[1];
        trackProcessorInfo->readCursor = tuple[0];

        createThread(trackProcessor, trackProcessorInfo);
    }
    else
    {
    }
}
void stopPlayback(State* state)
{
    HANDLE exitSemaphore = state->exitSemaphore;
    uint outputLoaderNumber = state->outputLoaderNumber;
    ReleaseSemaphore(exitSemaphore, (long)outputLoaderNumber + 1, 0);
    waitForSemaphore(exitSemaphore);

    state->outputLoaderNumber = 0;
    state->outputSet = 0;
    state->inputSet = 0;
    state->outputLoaderNumber = 0;
    state->audioClipArrayHandle = 0;

    void* inputTrackArrayHandle = state->inputTrackArrayHandle;
    resetArray(inputTrackArrayHandle);
    freeMemory(state->buffer);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_CREATECHILD:
        {
            createChild(state, window, wParam);
            break;
        }
        case WM_PAINT:
        {
            paint(window);
            break;
        }
        case WM_PREPARETOPLAY:
        {
            prepareToPlay(state, window, wParam, lParam);
            break;
        }
        case WM_SETINPUT:
        {
            setInput(state, window, wParam);
            break;
        }
        case WM_STARTOUTPUTLOADER:
        {
            startOutputLoader(state, wParam);
            break;
        }
        case WM_PLAY:
        {
            startProcessor(state, wParam, lParam);
            break;
        }
        case WM_PAUSE:
        {
            stopPlayback(state);
            break;
        }
        case WM_NCHITTEST:
        {
            return HTTRANSPARENT;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
