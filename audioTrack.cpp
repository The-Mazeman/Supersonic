#include "header.h"
#include "audioTrack.h"

#define INT16AVX2FRAMECOUNT 8

START_SCOPE(audioTrack)
void exchangeHandle(HANDLE* waitHandle, uint to, uint from)
{
	HANDLE temp = waitHandle[to];
	waitHandle[to] = waitHandle[from];
	waitHandle[from] = temp;
}
void chooseClip(AudioClip** selectedClip, AudioClip** clipList, uint clipCount, uint* loadCase)
{
	uint audioFrameCount = globalState.audioFrameCount;
	uint64 readCursor = globalState.readCursor * (uint64)audioFrameCount;
	sint64 framesPerPixel = globalState.framesPerPixel;
	AudioClip* currentClip = {};
	for (uint i = 0; i != clipCount; ++i)
	{
		currentClip = clipList[i];
		uint64 startFrame = (uint64)(currentClip->x * framesPerPixel);
		uint64 endFrame = startFrame + currentClip->frameCount;
		if (readCursor <= endFrame)
		{
			break;
		}
	}
	if (currentClip == 0)
	{
		*loadCase = 0;
	}
	else
	{
		*selectedClip = currentClip;
		uint64 startFrame = (uint64)(currentClip->x * framesPerPixel);
		uint64 endFrame = startFrame + currentClip->frameCount;
		if (readCursor < startFrame)
		{
			*loadCase = 1;
		}
		else if (readCursor == startFrame)
		{
			*loadCase = 2;
		}
		else if (readCursor == endFrame)
		{
			*loadCase = 3;
		}
		else
		{
			*loadCase = 4;
		}
	}
}
void calculateOffset(AudioClip* selectedClip)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint audioFrameCount = globalState.audioFrameCount;

	uint64 start = (uint64)(selectedClip->x * framesPerPixel);
	selectedClip->startFrame = start / audioFrameCount;
	selectedClip->startOffset = start % audioFrameCount;

	uint64 end = start + selectedClip->frameCount;
	selectedClip->endFrame = end / audioFrameCount;
	selectedClip->endOffset = end % audioFrameCount;
}
void prepareClip(AudioClip* selectedClip)
{
	uint audioFrameCount = globalState.audioFrameCount;
	uint64 readCursor = globalState.readCursor;
	uint64 startFrame = selectedClip->startFrame;

	uint64 offset = {};
	if (startFrame < readCursor)
	{
		offset = readCursor - startFrame;
		offset *= audioFrameCount;
	}
	short* sampleChunk = (short*)selectedClip->waveFile.sampleChunk;
	sampleChunk += offset * 2;
	selectedClip->start = sampleChunk;
}
void checkStartFrame(uint64 startFrame, HANDLE* waitHandle)
{
	uint64 readCursor = globalState.readCursor;
	if (readCursor == startFrame)
	{
		exchangeHandle(waitHandle, 1, 2);
	}
}
void alignStart(short** sampleChunk, short** buffer, uint startOffset, uint trackCount)
{
	uint frameCount = globalState.audioFrameCount;
	uint iterationCount = frameCount / INT16AVX2FRAMECOUNT;

	uint framesToZeroAVX2 = startOffset / 8;
	uint leftOver = startOffset % 8;
	__m256i* busBufferAVX2 = (__m256i*) * buffer;
	for (uint i = 0; i != framesToZeroAVX2; ++i)
	{
		*busBufferAVX2 = {};
		++busBufferAVX2;
	}
	short* busBuffer = (short*)busBufferAVX2;
	for (uint i = 0; i != leftOver; ++i)
	{
		*busBuffer = 0;
		++busBuffer;
	}
	uint fillCount = 8 - leftOver;
	short* sample = *sampleChunk;
	for (uint i = 0; i != fillCount; ++i)
	{
		*busBuffer = *sample;
		++busBuffer;
		++sample;
	}

	iterationCount -= framesToZeroAVX2 + 1;
	busBufferAVX2 = (__m256i*)busBuffer;
	__m256i* sampleAVX2 = (__m256i*)sample;
	for (uint i = 0; i != iterationCount; ++i)
	{
		*busBufferAVX2 = *sampleAVX2;
		++busBufferAVX2;
		++sampleAVX2;
	}
	*sampleChunk = (short*)sampleAVX2;
	*buffer = (short*)busBufferAVX2;
}
void load(short** sampleChunk, short** buffer, uint trackCount)
{
	uint frameCount = globalState.audioFrameCount;
	uint iterationCount = frameCount / INT16AVX2FRAMECOUNT;
	__m256i* sampleAVX2 = (__m256i*) * sampleChunk;
	__m256i* busBufferAVX2 = (__m256i*) * buffer;
	for (uint i = 0; i != iterationCount; ++i)
	{
		*busBufferAVX2 = *sampleAVX2;
		++busBufferAVX2;
		++sampleAVX2;
	}
	*sampleChunk = (short*)sampleAVX2;
	*buffer = (short*)busBufferAVX2;
}
void alignEnd(short** sampleChunk, short** buffer, uint endOffset, uint trackCount)
{
	uint frameCount = globalState.audioFrameCount;
	uint iterationCount = frameCount / INT16AVX2FRAMECOUNT;
	uint framesToFillAVX2 = endOffset / 8;
	uint leftOver = endOffset % 8;
	__m256i* busBufferAVX2 = (__m256i*) * buffer;
	__m256i* sampleAVX2 = (__m256i*) * sampleChunk;
	for (uint i = 0; i != framesToFillAVX2; ++i)
	{
		*busBufferAVX2 = *sampleAVX2;
		++busBufferAVX2;
		++sampleAVX2;
	}
	short* busBuffer = (short*)busBufferAVX2;
	short* sample = (short*)sampleAVX2;
	for (uint i = 0; i != leftOver; ++i)
	{
		*busBuffer = *sample;
		++busBuffer;
		++sample;
	}
	uint zeroFrameCount = 8 - leftOver;
	for (uint i = 0; i != zeroFrameCount; ++i)
	{
		*busBuffer = 0;
		++busBuffer;
	}
	iterationCount -= framesToFillAVX2 + 1;
	busBufferAVX2 = (__m256i*)busBuffer;
	for (uint i = 0; i != iterationCount; ++i)
	{
		*busBufferAVX2 = {};
		++busBufferAVX2;
	}
	*buffer = (short*)busBufferAVX2;
}
void zeroFillBuffer(short** buffer, uint iterationCount, uint* zeroFillCount)
{
	if (zeroFillCount == 0)
	{
		return;
	}
	__m256* bufferAVX2 = (__m256*)buffer;
	for (uint i = 0; i != iterationCount; ++i)
	{
		*bufferAVX2 = {};
	}
	--zeroFillCount;
}
void loadSample(AudioClip* selectedClip, short* buffer, uint trackCount, HANDLE* waitHandle)
{
	uint64 startFrame = selectedClip->startFrame;
	uint64 endFrame = selectedClip->endFrame;
	uint startOffset = selectedClip->startOffset;
	uint endOffset = selectedClip->endOffset;
	short* sampleChunk = (short*)selectedClip->waveFile.sampleChunk;

	uint running = 1;
	while(running)
	{
		uint signal = WaitForMultipleObjects(5, waitHandle, 0, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{
				//waitForStart();
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				alignStart(&sampleChunk, &buffer, startOffset, trackCount);
				break;
			}
			case WAIT_OBJECT_0 + 2:
			{
				load(&sampleChunk, &buffer, trackCount);
				break;
			}
			case WAIT_OBJECT_0 + 3:
			{
				alignEnd(&sampleChunk, &buffer, endOffset, trackCount);
				break;
			}
			case WAIT_OBJECT_0 + 4:
			{
				running = 0;
				break;
			}
		}
	}
}
DWORD WINAPI trackLoader(LPVOID parameter)
{
	State* state = (State*)parameter;
	AudioClip** clipList = state->clipList;

	uint zeroFillCount = {};
	uint trackCount = state->trackCount;
	uint clipCount = state->clipCount;
	uint frameCount = globalState.audioFrameCount;

	AudioClip* selectedClip = {};
	RingBuffer* busBuffer = state->busBuffer;
	short* buffer = (short*)busBuffer->start;

	HANDLE loadEvent = state->loadEvent;
	HANDLE exitSemaphore = state->exitSemaphore;
	HANDLE waitHandle[] = { loadEvent, exitSemaphore };

	uint loadCase = {};
	chooseClip(&selectedClip, clipList, clipCount, &loadCase);
	if(loadCase == 0)
	{
		return 0;
	}

	--loadCase;
	HANDLE dummyEvent;
	createEvent(0, &dummyEvent);
	HANDLE loadCaseEvent[] = {dummyEvent, dummyEvent, dummyEvent, dummyEvent, exitSemaphore};
	loadCaseEvent[loadCase] = loadEvent;

	uint running = 1;
	while (running)
	{
		uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
		switch(signal)
		{
			case WAIT_OBJECT_0:
			{
				calculateOffset(selectedClip);
				prepareClip(selectedClip);
				loadSample(selectedClip, buffer, trackCount, loadCaseEvent);
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				running = 0;
				break;
			}
		}
	}
	return 0;
}
void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	SetProp(window, L"state", state);
}
void insertClip(State* state, AudioClip* audioClip, uint position, uint clipCount)
{
	AudioClip** list = state->clipList;
	uint newClipCount = clipCount + 1;
	AudioClip** newList = {};
	allocateSmallMemory(sizeof(AudioClip*) * newClipCount, (char**)&newList);

	memcpy(newList, list, sizeof(AudioClip*) * position);
	newList[position] = audioClip;

	uint rest = clipCount - position;
	memcpy(&newList[position + 1], &list[position], sizeof(AudioClip*) * rest);
	if (list)
	{
		freeSmallMemory(list);
	}
	//state->clipList = newList;
}
void addClip(HWND window, WPARAM wParam)
{
	State* state = (State*)GetProp(window, L"state");
	AudioClip* audioClip = (AudioClip*)wParam;
	AudioClip** clipList = state->clipList;
	uint position = state->clipCount - 1;
	clipList[position] = audioClip;

	++state->clipCount;
}
void startLoader(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	createThread(trackLoader, state, 0);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
			break;
		}
		case WM_FILEDROP:
		{
			addClip(window, wParam);
		}
		case WM_STARTLOADER:
		{
			startLoader(window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE