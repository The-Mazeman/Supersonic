#pragma once
#include "platformWindows.hpp"
#include "dynamicArray.hpp"
#include "define.hpp"

struct BufferLoaderInfo
{
    float* inputBuffer;
    float* outputBuffer;
    HANDLE finishSemaphore;
    HANDLE startEvent;
    HANDLE exitSemaphore;
    uint loaderCount;
    uint loaderPosition;
    uint frameCount;
    uint padding;
};

void createBuffer(uint loaderCount, uint frameCount, float** buffer);
void startBufferLoader(HWND* trackArray, uint trackCount, BufferLoaderInfo* loaderInfo);
DWORD WINAPI bufferLoader(LPVOID parameter);
void accumulateFrame(float* input, float* output, uint iterationCount, uint inputLoaderCount);
void copySample(float** input, float* output, uint iterationCount);
