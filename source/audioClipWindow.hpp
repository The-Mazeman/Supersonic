
#pragma once
#include "dataType.hpp"
#include "define.hpp"
#include "platformWindows.hpp"
#include "graphicsGDI.hpp"
#include "dynamicArray.hpp"
#include "audioClip.hpp"
#include "waveformWindow.hpp"

START_SCOPE(audioClipWindow)

struct State
{
    AudioClip* audioClip;
};

void create(AudioClip* audioClip, RECT* boundingBox, HWND parent, HWND* window);

END_SCOPE
