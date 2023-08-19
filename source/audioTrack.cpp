#include "header.h"
#include "audioTrack.h"

START_SCOPE(audioTrack)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* audioTrack)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->clipCount = 0;
	state->trackControl.gain = _mm256_set1_ps(1.0f);

    createWindowClass(L"audioTrackWindowClass", windowCallback);
    createChildWindow(L"audioTrackWindowClass", window, audioTrack, state);

	SendMessage(window, WM_ASSIGNBUS, (WPARAM)*audioTrack, 0);
}
void addClip(State* state, WPARAM wParam)
{
	AudioClip** audioClip = (AudioClip**)wParam;
	uint clipCount = state->clipCount;
	state->clipList[clipCount] = *(*audioClip);
	state->clipList[clipCount + 1] = {};

    *audioClip = &state->clipList[clipCount];
	++state->clipCount;
}
void chooseClip(State* state, AudioClip** audioClip)
{
	uint64 readCursor = globalState.readCursor;
	uint clipCount = state->clipCount;
	AudioClip* clipList = state->clipList;
	for(uint i = 0; i != clipCount; ++i)
	{
		uint64 endFrame = clipList[i].endFrame;
		if (readCursor < endFrame)
		{
			*audioClip = &clipList[i];
			break;
		}
	}
}
void loadSample(float* sample, float** buffer, uint iterationCount, uint trackCount)
{
	__m256* sourceAVX2 = (__m256*)sample;
	__m256* destinationAVX2 = (__m256*)*buffer;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, *sourceAVX2);
		destinationAVX2 += trackCount;
		++sourceAVX2;
	}
	*buffer = (float*)destinationAVX2;
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
void fillZero(float** destination, uint iterationCount, uint trackCount)
{
	__m256 zero = _mm256_setzero_ps();
	__m256* destinationAVX2 = (__m256*)*destination;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, zero);
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
	uint64 start = audioClip->startFrame;
	float* sampleChunk = audioClip->waveFile.sampleChunk;
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
		uint64 offset = readCursor - start;
		uint64 loadFrameCount = globalState.audioEndpointFrameCount / 2;
		uint64 offsetFrameCount = offset * loadFrameCount;
		uint channelCount = audioClip->waveFile.header.channelCount;
		sampleChunk += (channelCount * offsetFrameCount);
		*loadCase = 2;
	}
	*sample = sampleChunk;
}
void selectNextClip(AudioClip** audioClip, uint* caseToSwitch)
{
	AudioClip* nextClip = *audioClip + 1;
	if(nextClip->startFrame == 0)
	{
		*caseToSwitch = 4;
		*audioClip = 0;
	}
	else
	{
		audioClip += 1;
	}
}
void fillProcessingBuffer(float** intput, float* output, uint iterationCount)
{
	__m256* sourceAVX2 = (__m256*)*intput;
	__m256* destinationAVX2 = (__m256*)output;
	for (uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)destinationAVX2, *sourceAVX2);
		++destinationAVX2;
		++sourceAVX2;
	}
	*intput = (float*)sourceAVX2;
}
void processAudioEffect(AudioEffect* audioEffectList, uint effectCount, float* sample, uint iterationCount)
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
void load(State* state, AudioClip* audioClip, float* sample, uint loadCase)
{
	Loader* trackLoader = &state->loader;
	HANDLE loadEvent = trackLoader->loadEvent;
	HANDLE exitEvent = trackLoader->exitSemaphore;
	HANDLE finishSemaphore = trackLoader->finishSemaphore;
	SetEvent(loadEvent);

	RingBuffer* outputBuffer = &trackLoader->buffer;
	uint bufferOffset = trackLoader->trackNumber * AVX2_FRAME_SIZE;
	float* destination = (float*)(trackLoader->buffer.start + bufferOffset);

	uint loadFrameCount = 1024;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = loadFrameCount / framesPerAVX2;
	uint bufferSize = sizeof(float) * 2 * loadFrameCount;

	float* processingBuffer = {};
	allocateSmallMemory(bufferSize, (void**)&processingBuffer);

	uint trackCount = trackLoader->trackCount;
	uint* finishCount = trackLoader->finishCount;

	uint64 startFrame = {};
	uint64 endFrame = {};
	uint caseToSwitch = 4;
	if(audioClip)
	{
		startFrame = audioClip->startFrame;
		endFrame = audioClip->endFrame - 1;
		caseToSwitch = 1;
	}
	uint64 readCursor = globalState.readCursor;
	uint zeroFillCount = 2;
	AudioEffect* audioEffectList = state->audioEffectList;
	audioEffectList->process = processGain;
	audioEffectList->state = &state->trackControl.gain;
	uint effectCount = 1;
	HANDLE waitHandle[] = {loadEvent, exitEvent};
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
						fillProcessingBuffer(&sample, processingBuffer, iterationCount);
						processAudioEffect(audioEffectList, effectCount, processingBuffer, iterationCount);
						loadSample(processingBuffer, &destination, iterationCount, trackCount);
						checkClipEnd(readCursor, endFrame, &loadCase);
						boundCheck(outputBuffer, (void**)&destination, bufferOffset); 
						break;
					}
					case 3:
					{
						selectNextClip(&audioClip, &caseToSwitch);
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

	load(state, audioClip, sample, loadCase);
			
	return 0;
}
void startLoader(State* state, WPARAM wParam)
{
	state->loader = *(Loader*)wParam;
	createThread(sampleLoader, (LPVOID)state, 0);
}
void setControl(State* state, HWND window)
{
	HWND parent = GetAncestor(window, GA_PARENT);
	TrackControl* trackControl = &state->trackControl;
	SendMessage(parent, WM_SETCONTROL, (WPARAM)trackControl, 0);
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
		case WM_SETCONTROL:
		{
			setControl(state, window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
