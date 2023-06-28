#include "header.h"
#if 0
void insertClip(AudioTrack* audioTrack, AudioClip* audioClip, uint position, uint clipCount)
{
	AudioClip** list = audioTrack->clipList;
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
	audioTrack->clipList = newList;
}
void addClip(AudioTrack* audioTrack, WPARAM wParam)
{
	AudioClip* audioClip = (AudioClip*)wParam;
	AudioClip** clipList = audioTrack->clipList;
	uint clipCount = audioTrack->clipCount;
	uint position = {};
	for (uint i = 0; i != clipCount; ++i)
	{
		if (audioClip->x < clipList[i]->x)
		{
			break;
		}
		++position;
	}
	insertClip(audioTrack, audioClip, position, clipCount);
	++audioTrack->clipCount;
}
void exchangeHandle(HANDLE* waitHandle, uint to, uint from)
{
	HANDLE temp = waitHandle[to];
	waitHandle[to] = waitHandle[from];
	waitHandle[from] = temp;
}
void chooseClip(AudioClip** selectedClip, AudioClip** clipList, uint clipCount, HANDLE* waitHandle)
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
		exchangeHandle(waitHandle, 0, 5);
	}
	else
	{
		*selectedClip = currentClip;
		uint64 startFrame = (uint64)(currentClip->x * framesPerPixel);
		uint64 endFrame = startFrame + currentClip->frameCount;
		if (readCursor < startFrame)
		{
			exchangeHandle(waitHandle, 0, 1);
		}
		else if (readCursor == startFrame)
		{
			exchangeHandle(waitHandle, 0, 2);
		}
		else if (readCursor == endFrame)
		{
			exchangeHandle(waitHandle, 0, 4);
		}
		else
		{
			exchangeHandle(waitHandle, 0, 3);
		}
	}
}

void calculateOffset(AudioClip* selectedClip, uint64* startFrame, uint* startOffset, uint64* endFrame, uint* endOffset)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint audioFrameCount = globalState.audioFrameCount;

	uint64 start = (uint64)(selectedClip->x * framesPerPixel);
	*startFrame = start / audioFrameCount;
	*startOffset = start % audioFrameCount;

	uint64 end = start + selectedClip->frameCount;
	*endFrame = end / audioFrameCount;
	*endOffset = end % audioFrameCount;
}
void prepareClip(short** sampleChunk, AudioClip* selectedClip, uint64 startFrame)
{
	uint audioFrameCount = globalState.audioFrameCount;
	uint64 readCursor = globalState.readCursor;
	uint64 offset = {};
	if (startFrame < readCursor)
	{
		offset = readCursor - startFrame;
		offset *= audioFrameCount;
	}
	*sampleChunk = (short*)selectedClip->waveFile.sampleChunk;
	*sampleChunk += offset * 2;
}
void checkStartFrame(uint64 startFrame, HANDLE* waitHandle)
{
	uint64 readCursor = globalState.readCursor;
	if (readCursor == startFrame)
	{
		exchangeHandle(waitHandle, 1, 2);
	}
}
void alignStart(short** sampleChunk, short** buffer, uint startOffset, uint iterationCount)
{
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
void load(short** sampleChunk, short** buffer, uint iterationCount)
{
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
void alignEnd(short** sampleChunk, short** buffer, uint endOffset, uint iterationCount)
{
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

START_SCOPE(int16)

DWORD WINAPI trackLoader(LPVOID parameter)
{
	AudioTrack* audioTrack = (AudioTrack*)parameter;
	AudioClip** clipList = audioTrack->clipList;

	uint64 startFrame = {};
	uint64 endFrame = {};
	uint startOffset = {};
	uint endOffset = {};

	uint zeroFillCount = {};

	uint clipCount = audioTrack->clipCount;
	uint frameCount = globalState.audioFrameCount;
	uint iterationCount = frameCount / 8;

	AudioClip* selectedClip = {};
	short* sampleChunk = {};
	RingBuffer* busBuffer = audioTrack->busBuffer;
	short* buffer = (short*)busBuffer->start;

	HANDLE loadEvent = audioTrack->loadEvent;
	HANDLE exitSemaphore = audioTrack->exitSemaphore;
	HANDLE busSemaphore = audioTrack->busSemaphore;

	HANDLE dummyEvent;
	createEvent(0, &dummyEvent);

	HANDLE waitHandle[] = { loadEvent,  dummyEvent, dummyEvent, dummyEvent, dummyEvent, exitSemaphore };
	while (1)
	{
		uint signal = WaitForMultipleObjects(3, waitHandle, 0, INFINITE);
		switch (signal)
		{
		case WAIT_OBJECT_0 + 0:
		{
			chooseClip(&selectedClip, clipList, clipCount, waitHandle);
			calculateOffset(selectedClip, &startFrame, &startOffset, &endFrame, &endOffset);
			prepareClip(&sampleChunk, selectedClip, startFrame);
			zeroFillCount = 2;
			break;
		}
		case WAIT_OBJECT_0 + 1:
		{
			checkStartFrame(startFrame, waitHandle);
			zeroFillBuffer(&buffer, iterationCount, &zeroFillCount);
			break;
		}
		case WAIT_OBJECT_0 + 2:
		{
			alignStart(&sampleChunk, &buffer, startOffset, frameCount);
			exchangeHandle(waitHandle, 1, 2);
			break;
		}
		case WAIT_OBJECT_0 + 3:
		{
			load(&sampleChunk, &buffer, iterationCount);
			break;
		}
		case WAIT_OBJECT_0 + 4:
		{
			alignEnd(&sampleChunk, &buffer, endOffset, frameCount);
			exchangeHandle(waitHandle, 3, 0);
			break;
		}
		case WAIT_OBJECT_0 + 5:
		{
			return 0;
		}
		}
	}
	return 0;
}

END_SCOPE

void startLoader(AudioTrack* audioTrack)
{
	createThread(int16::trackLoader, audioTrack, 0);
}
void trackCallback(UINT message, WPARAM wParam, LPARAM lParam, AudioTrack* audioTrack)
{
	switch (message)
	{
	case WM_FILEDROP:
	{
		addClip(audioTrack, wParam);
		break;
	}
	case WM_STARTLOADER:
	{
		startLoader(audioTrack);
		break;
	}
	}
}
DWORD WINAPI trackThread(LPVOID parameter)
{
	AudioTrack* audioTrack = (AudioTrack*)parameter;
	HANDLE loadEvent = audioTrack->loadEvent;

	MSG message;
	PeekMessage(&message, NULL, WM_USER, WM_USER, PM_NOREMOVE);
	SetEvent(loadEvent);
	while (1)
	{
		GetMessage(&message, 0, 0, 0);
		trackCallback(message.message, message.wParam, message.lParam, audioTrack);
	}

	return 0;
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
DWORD WINAPI wasapiController(LPVOID parameter)
{
	State* state = (State*)parameter;
	HANDLE audioCallback = state->wasapi.audioCallback;
	HANDLE loadEndpointEvent = state->loadEndpointEvent;
	IAudioClient* audioClient = state->wasapi.audioClient;
	uint frameCount = state->wasapi.bufferFrameCount / 2;
	while (1)
	{
		uint signal = WaitForSingleObjectEx(audioCallback, INFINITE, 0);
		sendLoadSignal(audioClient, loadEndpointEvent, frameCount);
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
DWORD WINAPI endpointLoader32BitFloat(LPVOID parameter)
{
	State* state = (State*)parameter;

	HANDLE loadEndpointEvent = state->loadEndpointEvent;
	IAudioRenderClient* renderClient = state->wasapi.renderClient;
	RingBuffer* outputBuffer = &state->outputBuffer;
	float* outputBufferPointer = (float*)outputBuffer->start;
	uint frameCount = state->wasapi.bufferFrameCount / 2;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;
	while (1)
	{
		uint signal = WaitForSingleObjectEx(loadEndpointEvent, INFINITE, 0);
		loadOutput(renderClient, &outputBufferPointer, iterationCount, frameCount);
		bufferBoundCheck(outputBuffer, (void**)&outputBufferPointer, 0);
	}
	return 0;
}
void createOutputBuffer(State* state)
{
	uint bufferFrameCount = state->wasapi.bufferFrameCount / 2;
	uint bufferFrameSize = state->wasapi.format.blockAlign;
	uint bufferMemorySize = bufferFrameCount * bufferFrameSize;

	char* outputBuffer;
	allocateSmallMemory(bufferMemorySize, &outputBuffer);

	state->outputBuffer.start = outputBuffer;
	state->outputBuffer.end = outputBuffer + bufferMemorySize;
}
#endif