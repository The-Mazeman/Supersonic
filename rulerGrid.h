#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(rulerGrid)

void paintWindow(HWND window);
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE
