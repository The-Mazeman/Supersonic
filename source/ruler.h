#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(ruler)

struct State
{
    int x;
    int y;
    int height;
    int padding;
};

void create(HWND parent, HWND* child);

END_SCOPE
