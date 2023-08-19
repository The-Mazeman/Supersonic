#pragma once
#include "header.h"

struct GlobalState
{
	sint64 framesPerPixel;

	int trackHeight;
	int rulerHeight;

	int sidebarWidth;
	int topbarHeight;

	int trackControlWidth;
	int trackControlHeight;

    int offsetX;
    int offsetY;

	uint sampleRate;
	uint trackCount;

	uint audioEndpointFrameCount;
	int padding;
	uint64 readCursor;
};
extern GlobalState globalState;
