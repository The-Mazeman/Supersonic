#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "globalState.hpp"

START_SCOPE(rulerWindow)

struct State
{
    GlobalState* globalState;
	int x;
	int y;
    int scalar;
    int padding;
};

void create(GlobalState* globalState, HWND parent, HWND* window);

END_SCOPE
