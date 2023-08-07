#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "textbox.h"

START_SCOPE(trackHeader)

struct State
{
	String name;
	uint clipCount;
	int padding;
};

void create(HWND window, HWND* trackHeader);

END_SCOPE
