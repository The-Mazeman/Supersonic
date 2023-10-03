#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "audioTrack.h"
#include "bus.h"
#include "wasapi.h"
#include "trackControl.h"
#include "masterBus.h"
#include "dynamicArray.h"

START_SCOPE(audioEngine)

struct State
{
	HWND wasapi;
	HWND trackControl;

	void* busArrayHandle;

	HANDLE outputLoadEvent;
	void* audioTrackArrayHandle;
	void* trackProcessorStartEventArrayHandle;

	HWND timelineCursor;
	sint64 timelineCursorX;
};

void create(HWND window, HWND* audioEngine);

END_SCOPE
