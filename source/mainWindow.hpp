#pragma once 

#include "define.hpp"
#include "dataType.hpp"
#include "dynamicArray.hpp"
#include "globalState.hpp"
#include "platformWindows.hpp"
#include "titleBarWindow.hpp"
#include "rulerWindow.hpp"
#include "sidebarWindow.hpp"
#include "clipAreaWindow.hpp"
#include "graphicsGDI.hpp"
#include "audioClip.hpp"
#include "audioEngine.hpp"

START_SCOPE(mainWindow)

struct State
{
    GlobalState* globalState;
    uint trackCount;
    uint playing;
    HWND titleBar;
    HWND ruler;
    HWND clipArea;
    HWND sidebar;
    void* audioClipContainerArrayHandle;
};

void create(GlobalState*);

END_SCOPE
