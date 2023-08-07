#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "audioClip.h"
#include "timelineCursor.h"

START_SCOPE(clipArea)

struct State
{
    int x;
    int y;
    int height;
    int padding;
    HWND timelineCursor;

};

void create(HWND parent, HWND* child);

END_SCOPE
