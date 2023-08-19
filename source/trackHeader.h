#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "textbox.h"

START_SCOPE(trackHeader)

struct State
{
	HWND textbox;
	HWND audioTrack;
};

void create(HWND window, HWND* trackHeader, HWND audioTrack);

END_SCOPE
