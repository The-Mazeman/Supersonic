#include "include.hpp"
#include "globalState.hpp"

void setGlobalState(GlobalState* globalState)
{
    globalState->framesPerPixel = 512;
    globalState->trackHeight = 128;
    globalState->rulerHeight = 16;
    globalState->sidebarWidth = 128;
    globalState->titleBarHeight = 16;
    globalState->trackControlWidth = 128;
    globalState->trackControlHeight = 512;
    globalState->sampleRate = 48000;
    globalState->trackCount = 0;
    globalState->audioEndpointFrameCount = 1024;
    globalState->framesPerPixel = 512;
    globalState->offsetX = 0;
    globalState->offsetY = 0;
    globalState->readCursor = 0;
}

