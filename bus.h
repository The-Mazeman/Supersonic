#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(bus)

struct State
{
	HWND inputLoaderArray[16];
	HANDLE inputLoadEventArray[16];
	HANDLE outputLoadEvent;
	HANDLE outputExitEvent;
	HANDLE inputExitSemaphore;
	HANDLE inputFinishSemaphore;
	HANDLE outputFinishSemaphore;

	RingBuffer inputBuffer;
	RingBuffer outputBuffer;

	uint inputTrackCount;
	uint outputTrackCount;
	uint inputFinishCount;
	uint outputTrackNumber;
	uint* outputFinishCount;
};

void create(HWND parent, HWND* bus);

END_SCOPE
