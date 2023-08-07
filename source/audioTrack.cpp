#include "header.h"
#include "audioTrack.h"

START_SCOPE(audioTrack)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* audioTrack)
{
    createWindowClass(L"audioTrackWindowClass", windowCallback);
    createChildWindow(L"audioTrackWindowClass", window, audioTrack);
}
void addClip(State* state, WPARAM wParam)
{
	AudioClip** audioClip = (AudioClip**)wParam;
	state->clipList[0] = *(*audioClip);
    *audioClip = &state->clipList[0];

	++state->clipCount;
}
void chooseClip(State* state, AudioClip** audioClip)
{
	uint64 readCursor = globalState.readCursor;
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint clipCount = state->clipCount;
	AudioClip* clipList = state->clipList;
	for(uint i = 0; i != clipCount; ++i)
	{
		uint64 endFrame = clipList[i].x * (uint64)framesPerPixel + clipList[i].frameCount;
		endFrame /= loadFrameCount;

		if (readCursor <= endFrame)
		{
			*audioClip = &clipList[i];
			break;
		}
	}
}
void loadSample(float** sample, float** buffer, uint iterationCount, uint trackCount)
{
	__m256* source = (__m256*)*sample;
	__m256* destination = (__m256*)*buffer;
	for(uint i = 0; i != iterationCount; ++i)
	{
		*destination = *source;
		destination += (trackCount);
		++source;
	}
	*sample = (float*)source;
	*buffer = (float*)destination;
}
void checkStart(uint64 startFrame, uint64 readCursor, uint* loadCase)
{
	if(readCursor == startFrame)
	{
		*loadCase = 2;
	}
}
void checkClipEnd(float* sample, float* clipEnd, uint* loadCase)
{
	if(sample == clipEnd)
	{
		*loadCase = 3;
	}
}
void fillZero(float** destination, uint iterationCount, uint trackCount)
{
	__m256* destinationAVX2 = (__m256*)*destination;
	for(uint i = 0; i != iterationCount; ++i)
	{
		*destinationAVX2 = {};
		destinationAVX2 += trackCount;
	}
	*destination = (float*)destinationAVX2;
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
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint64 start = audioClip->x * (uint64)framesPerPixel;
	start /= loadFrameCount;
	audioClip->startFrame = start;

  	uint64 frameCount = audioClip->width * (uint64)framesPerPixel;
	audioClip->frameCount = frameCount;

	*sample = audioClip->waveFile.sampleChunk;
	if(readCursor < start)
	{
		*loadCase = 0;
	}
	else if(readCursor == start)
	{
		*loadCase = 1;
	}
	else
	{
	}
}
void selectNextClip(AudioClip* audioClip, uint clipCount, uint* caseToSwitch)
{
	if(audioClip->id + 1 < clipCount)
	{
		audioClip += 1;
	}
	else
	{
		*caseToSwitch = 4;
		audioClip = 0;
	}

}
void load(Loader* trackLoader, AudioClip* audioClip, float* sample, uint loadCase, uint clipCount)
{
	HANDLE loadEvent = trackLoader->loadEvent;
	HANDLE exitEvent = trackLoader->exitSemaphore;
	HANDLE finishSemaphore = trackLoader->finishSemaphore;
	SetEvent(loadEvent);

	RingBuffer* outputBuffer = &trackLoader->buffer;
	uint bufferOffset = trackLoader->trackNumber * AVX2_FRAME_SIZE;
	float* destination = (float*)(trackLoader->buffer.start + bufferOffset);

	uint loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;

	uint64 frameCount = audioClip->frameCount / loadFrameCount;
	uint channelCount = audioClip->waveFile.header.channelCount;
	float* clipEnd = sample + (frameCount * loadFrameCount * channelCount);

	uint trackCount = trackLoader->trackCount;
	uint* finishCount = trackLoader->finishCount;
	uint64 startFrame = audioClip->startFrame;
	uint64 readCursor = globalState.readCursor;
	uint zeroFillCount = 2;
	HANDLE waitHandle[] = {loadEvent, exitEvent};
	uint running = 1;
	uint caseToSwitch = 1;
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
						fillZero(&destination, iterationCount, trackCount);
						boundCheck(outputBuffer, (void**)&destination, bufferOffset); 
						--zeroFillCount;
						checkFillCount(&loadCase, zeroFillCount, caseToSwitch);
					}
					case 1:
					{
						checkStart(startFrame, readCursor, &loadCase);
						break;
					}
					case 2:
					{
						loadSample(&sample, &destination, iterationCount, trackCount);
						checkClipEnd(sample, clipEnd, &loadCase);
						boundCheck(outputBuffer, (void**)&destination, bufferOffset); 
						break;
					}
					case 3:
					{
						selectNextClip(audioClip, clipCount, &caseToSwitch);
						prepareClip(audioClip, &sample, &loadCase);
						zeroFillCount = 2;
						break;
					}
					case 4:
					{
					}
				}
				break;
			}
			case WAIT_OBJECT_0 + 1:
			{
				running = 0;
			}
		}
		InterlockedIncrement(finishCount);
		ReleaseSemaphore(finishSemaphore, 1, 0);
	}
}
DWORD WINAPI sampleLoader(LPVOID parameter)
{
	State* state = (State*)parameter;
	AudioClip* audioClip = {};
	chooseClip(state, &audioClip);

	uint loadCase;
	float* sample;
	prepareClip(audioClip, &sample, &loadCase);

	uint clipCount = state->clipCount;
	Loader* loader = &state->loader;
	load(loader, audioClip, sample, loadCase, clipCount);
	return 0;
}
void startLoader(State* state, WPARAM wParam)
{
	state->loader = *(Loader*)wParam;

	createThread(sampleLoader, (LPVOID)state, 0);
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	state->clipCount = 0;

	HWND parent = GetAncestor(window, GA_PARENT);
	SendMessage(parent, WM_ASSIGNBUS, (WPARAM)window, 0);
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
		case WM_FILEDROP:
		{
			addClip(state, wParam);
			break;
		}
		case WM_STARTLOADER:
		{
			startLoader(state, wParam);
			break;
		}
		case WM_ASSIGNBUS:
		{
			//selectBus(window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
