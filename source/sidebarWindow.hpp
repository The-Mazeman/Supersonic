#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "globalState.hpp"
#include "contextMenuWindow.hpp"
#include "trackHeaderWindow.hpp"
#include "dynamicArray.hpp"
#include "audioWasapi.hpp"

START_SCOPE(sidebarWindow)

struct State
{
    GlobalState* globalState;
	int x;
	int y;
    uint globalSoloState;
    uint soloTrackCount;
    void* trackHeaderArrayHandle;
    void* audioTrackArrayHandle;
    void* busTrackArrayHandle;
    void* audioTrackStartEventArrayHandle;
    HWND contextMenu;
};

void create(GlobalState* globalState, HWND parent, HWND* window, HWND clipAreaWindow);

END_SCOPE
