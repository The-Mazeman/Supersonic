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
#include "labelWindow.hpp"
#include "audioUtilities.hpp"
#include "buttonWindow.hpp"

START_SCOPE(trackHeaderWindow)

struct State
{
    void* audioClipArrayHandle;
    HWND textbox;
    HWND gainParameter;
    HWND panParameter;
    HWND muteButton;
    HWND soloButton;
    HWND inputEnableButton;
    float* gainValue;
    float* panValue;
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
    HANDLE muteEvent;
    HANDLE dummyEvent;
};
struct BusProcessorInfo
{
    HANDLE finishSemaphore;
    HANDLE exitSemaphore;
    HANDLE muteEvent;
    HANDLE dummyEvent;
    float* buffer;
    HANDLE* loaderStartEventArray;
    uint outputLoaderCount;
    uint inputLoaderCount;
    uint frameCount;
    int padding;
};
struct TrackProcessorInfo
{
    void* audioClipArrayHandle;
    float* buffer;
    HANDLE startEvent;
    HANDLE exitSemaphore;
    HANDLE muteEvent;
    HANDLE dummyEvent;
    HANDLE* loaderStartEventArray;
    uint outputLoaderCount;
    uint frameCount;
    uint64 readCursor;
    float gainValue;
    float panValue;
};

void create(String* trackName, HWND parent, RECT* boundingBox, HWND* window);

END_SCOPE
