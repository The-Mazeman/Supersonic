#pragma once
#include "header.h"
#include "platform.h"
#include "waveFile.h"
#include "globalState.h"
#include "waveform.h"

START_SCOPE(audioClip)
struct State
{
	AudioClip* audioClip;
};
void create(HWND window, HWND* audioClip);

END_SCOPE
