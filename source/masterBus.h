#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "dynamicArray.h"

START_SCOPE(masterBus)

struct State
{
	HANDLE bufferCompleteSemaphore;
    HANDLE exitSemaphore;
    HANDLE loadOutputEvent;
    HANDLE startBusLoaderEvent;

    HWND wasapi;
    void* inputLoaderArrayHandle;
	float* inputBuffer;

	uint outputLoaderCount;
	uint outputLoaderNumber;
	uint inputBufferCompleteCount;
    uint outputSet;
	uint inputSet;
	uint padding;
};

void create(HWND parent, HWND* bus, HWND wasapi);

END_SCOPE
