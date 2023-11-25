#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "dynamicArray.hpp"
#include "graphicsGDI.hpp"
#include "dataStructure.hpp"
#include "textboxWindow.hpp"
#include "audioClip.hpp"
#include "buffer.hpp"

START_SCOPE(trackHeaderWindow)

struct State
{
    void* audioClipArrayHandle;
    HWND textbox;
    float* buffer;
    void* inputTrackArrayHandle;
    void* outputBusNumberArrayHandle;
    void* startEventArrayHandle;
    uint outputSet;
    uint inputSet;
    uint outputLoaderNumber;
    uint inputLoaderCount;
    HANDLE finishSemaphore;
    HANDLE exitSemaphore;
};
struct BusProcessorInfo
{
    HANDLE finishSemaphore;
    HANDLE exitSemaphore;
    float* buffer;
    HANDLE* loaderStartEventArray;
    uint outputLoaderCount;
    uint inputLoaderCount;
    uint frameCount;
    int padding;
};
struct TrackProcessorInfo
{
    HANDLE startEvent;
    HANDLE exitSemaphore;
    float* buffer;
    HANDLE* loaderStartEventArray;
    uint outputLoaderCount;
    uint frameCount;
    uint64 readCursor;
    void* audioClipArrayHandle;
};

void create(String* trackName, HWND parent, RECT* boundingBox, HWND* window);

END_SCOPE
