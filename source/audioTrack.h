#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "dynamicArray.h"

START_SCOPE(audioTrack)

struct State
{	
	void* loaderTrackControlArrayHandle;
	void* trackControlArrayHandle;
    void* outputLoaderEventArrayHandle;
    void* outputBusArrayHandle;
	TrackControl* activeTrackControl;
	void* audioClipArrayHandle;
	HANDLE exitSemaphore;

	float* inputBuffer;
	uint effectCount;
	uint controlSet;
	uint outputLoaderCount;
	uint outputLoaderNumber;
};

void create(HWND window, HWND* audioTrack);

END_SCOPE
