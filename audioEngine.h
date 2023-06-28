#pragma once
#include "header.h"
#include "platform.h"
#include "audioClip.h"
#include "audioTrack.h"
#include "wasapi.h"
#include "globalState.h"

START_SCOPE(audioEngine)

struct State
{
	HWND wasapi;
	IAudioRenderClient* renderClient;

	HANDLE loadEvent;
	HANDLE exitSemaphore;
	HANDLE busSemaphore;
	
	RingBuffer endpointBuffer;
	RingBuffer* outputBuffer;

	HWND audioTrackArray[4];
	HANDLE trackLoadEvent[3];
	HWND timelineCursor;
	int timelineCursorX;
	int padding;
};

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE