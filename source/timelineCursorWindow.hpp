#pragma once
#include "define.hpp"
#include "dataType.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"

START_SCOPE(timelineCursorWindow)

struct State
{

};
void create(HWND parent, HWND* timelineCursor);

END_SCOPE

