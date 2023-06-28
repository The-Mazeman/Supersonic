#include "header.h"
#include "globalState.h"

GlobalState globalState;

void initializeGlobalState()
{
	globalState.framesPerPixel = 512;
	globalState.sampleRate = 48000;
	globalState.trackCount = 0;
	globalState.trackHeight = 128;
	globalState.rulerHeight = 16;
	globalState.sidebarWidth = 128;
	globalState.topbarHeight = 16;
}