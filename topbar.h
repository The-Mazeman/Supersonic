#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(topbar)

struct State
{
    int x;
    int y;
    int width;
    int height;
};

void create(HWND parent, HWND* child);

END_SCOPE
