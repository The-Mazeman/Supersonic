#pragma once
#include "header.h"

struct GlobalState
{
	sint64 framesPerPixel;
	int sampleRate;
	int trackCount;
	int trackHeight;
	int rulerHeight;
	int sidebarWidth;
	int topbarHeight;
	uint audioFrameCount;
	uint readCursor;
};
extern GlobalState globalState;

void initializeGlobalState();
