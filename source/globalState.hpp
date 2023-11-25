#pragma once
#include "dataType.hpp"

struct GlobalState
{
    uint framesPerPixel;
    uint trackHeight;
    uint rulerHeight;
    uint sidebarWidth;
    uint titleBarHeight;
    uint trackControlWidth;
    uint trackControlHeight;
    uint sampleRate;
    uint trackCount;
    uint audioEndpointFrameCount;
    uint offsetX;
    uint offsetY;
    uint64 readCursor;
    HWND audioEngine;
};

void setGlobalState(GlobalState* globalState);
