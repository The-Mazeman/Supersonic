#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"

START_SCOPE(buttonWindow)

struct State
{
    String string;
    uint pressed;
    uint buttonId;
};

void create(String* buttonString, uint buttonId, RECT* boundingBox, HWND parent, HWND* window);

END_SCOPE
