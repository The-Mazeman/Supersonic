#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(rectangle)

struct State
{
	WindowPosition* position;
};

void create(HWND parent, HWND* window, WindowPosition* position);

END_SCOPE