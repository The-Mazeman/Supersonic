#pragma once
#include "define.hpp"
#include "dataType.hpp"
#include "platformWindows.hpp"
#include "dataStructure.hpp"
#include "graphicsGDI.hpp"

START_SCOPE(contextMenuWindow)

struct State 
{
    HWND ownerWindow;
    String* menuOptionArray;
    uint menuOptionCount;
    int padding;
};

void create(HWND, String*, uint, HWND*);

END_SCOPE
