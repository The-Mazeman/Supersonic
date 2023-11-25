#pragma once

#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "dataStructure.hpp"

START_SCOPE(waveformWindow)

struct CreateWaveformInfo
{
    float* sampleChunk;
    float* waveformSampleChunk;
    uint framesPerPixel;
    uint width;
    HWND window;
};
struct State
{
    float* sampleChunk;
    uint64 frameCount;
    float* waveformSampleChunk;
    uint waveformFrameCount;
    int paddint;
};

void create(HWND parent, RECT* boundingBox, float* sampleChunk, uint64 frameCount);

END_SCOPE
