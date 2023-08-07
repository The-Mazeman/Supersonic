#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(timelineCursor)

struct State
{
	int x;
    int y;
};

void create(HWND parent, HWND* timelineCursor);
END_SCOPE
