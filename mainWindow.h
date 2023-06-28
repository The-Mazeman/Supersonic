#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(main)

struct State
{
	HWND topbar;
	HWND ruler;
	HWND sidebar;
	HWND clipArea;
	HWND audioEngine;

	int playing;
	int padding;
};

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE