#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(timelineCursor)

struct State
{
	int x;
	int padding;
	uint64 endpointDeviceFrequency;
	IAudioClock* audioClock;
};
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE