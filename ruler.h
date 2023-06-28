#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(ruler)

void paintWindow(HWND window);
void createGridWindow(HWND window);
void handleResize(HWND window, LPARAM lParam);
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE