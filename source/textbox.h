#pragma once
#include "header.h"
#include "platform.h"

START_SCOPE(textbox)

struct State
{
    String name;
};
void create(HWND window, HWND* textBox);

END_SCOPE
