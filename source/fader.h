#pragma once
#include "header.h"
#include "platform.h"
#include "rectangle.h"

START_SCOPE(fader)

struct State
{
	WindowPosition position;
	WindowPosition rectanglePosition;
	HWND rectangle;
	HWND parent;
	__m256* output;
	int upperLimit;
	int lowerLimit;

};

void create(HWND parent, HWND* window, WindowPosition* position);

END_SCOPE