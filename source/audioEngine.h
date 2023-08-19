#pragma once
#include "header.h"
#include "platform.h"
#include "audioTrack.h"
#include "wasapi.h"
#include "trackControl.h"
#include "globalState.h"

START_SCOPE(audioEngine)

struct State
{
	HWND wasapi;
	HWND trackControl;
	IAudioRenderClient* renderClient;

	HANDLE loadEvent;
	HANDLE endLoaderEvent;
	HANDLE exitSemaphore;
	HANDLE busSemaphore;
	
	RingBuffer endpointBuffer;

	HWND audioTrackArray[16];
	HANDLE trackLoadEvent[16];
	HWND timelineCursor;
	sint64 timelineCursorX;
};

void create(HWND window, HWND* audioEngine);

END_SCOPE
