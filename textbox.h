#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(textbox)

void create(HWND window, String* text, int width);
void paintWindow(HWND window);
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE
