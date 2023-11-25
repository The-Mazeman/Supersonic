#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "globalState.hpp"
#include "dynamicArray.hpp"
#include "audioClip.hpp"
#include "audioClipWindow.hpp"
#include "timelineCursorWindow.hpp"

START_SCOPE(clipAreaWindow)

struct State
{
    GlobalState* globalState;
	int x;
	int y;
    int scalar;
    int dropLocation;
    uint64 timelineCursorBasePosition;
    void* audioClipWindowContainerArrayHandle;
    HWND selectedChild;
    HWND timelineCursorWindow;
};

void create(GlobalState* globalState, HWND parent, HWND* window);

END_SCOPE
