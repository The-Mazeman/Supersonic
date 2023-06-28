#pragma once
#include "header.h"
#include "platform.h"
#include "audioClip.h"
#include "globalState.h"

START_SCOPE(audioTrack)

struct State
{	
	AudioClip* clipList[5];
	RingBuffer* busBuffer;
	HANDLE loadEvent;
	HANDLE exitSemaphore;
	uint* finishCount;

	uint clipCount;
	uint trackCount;
};

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE