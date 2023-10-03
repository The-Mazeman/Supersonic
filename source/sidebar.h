#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "trackHeader.h"

START_SCOPE(sidebar)

struct State
{
    int x;
    int y;
    int width;
    int padding;

    HWND trackHeaderArray[32];
};

void create(HWND parent, HWND* child);

END_SCOPE

