#pragma once
#include "header.h"
#include "platform.h"
#include "audioClip.h"

START_SCOPE(clipGrid)

struct State
{
	HWND timelineCursor;
};

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE
