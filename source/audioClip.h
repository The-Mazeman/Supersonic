#pragma once
#include "header.h"
#include "platform.h"
#include "waveFile.h"
#include "globalState.h"
#include "waveform.h"

START_SCOPE(audioClip)

struct State
{
	int x;
	int width;
	AudioClip* audioClip;
	HWND waveform;
};

void create(HWND parent, HWND* window, AudioClip* audioClip);

END_SCOPE
