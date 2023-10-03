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
	int upperLimit;
	int lowerLimit;
	uint faderId;
	uint padding;
};

void create(HWND, HWND*, WindowPosition*, uint);

END_SCOPE