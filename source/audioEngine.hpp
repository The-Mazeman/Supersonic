#pragma once
#include "platformWindows.hpp"
#include "define.hpp"
#include "dataType.hpp"
#include "dynamicArray.hpp"
#include "audioWasapi.hpp"

START_SCOPE(audioEngine)

struct State
{
    HWND wasapi;
};
void create(HWND parent, HWND* window);

END_SCOPE
