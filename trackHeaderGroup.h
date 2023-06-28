#pragma once
#include "header.h"
#include "platform.h"
//#include "audioClip.h"

START_SCOPE(trackHeaderGroup)

struct State
{
	HWND trackHeaderArray[64];
};

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE
