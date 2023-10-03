#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(bus)

struct State
{
	HANDLE bufferCompleteSemaphore;
    HANDLE busLoaderStartEventArray[4];
    HANDLE exitSemaphore;
    HWND inputLoaderArray[4];

	float* inputBuffer;

	uint inputLoaderCount;
	uint outputLoaderCount;
	uint outputLoaderNumber;
	uint inputBufferCompleteCount;
    uint outputSet;
    uint outputBusArray[2];
    uint outputBusCount;
    uint inputSet;
	uint padding;
};

void create(HWND parent, HWND* bus);

END_SCOPE
