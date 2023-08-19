#pragma once
#include "header.h"
#include "platform.h"
#include "waveFile.h"
#include "globalState.h"

START_SCOPE(waveform)

struct State
{
	int startOffset;
	int endOffset;
    WaveFile waveFile;
    float* waveform;
};

void create(HWND window, HWND* waveform, WaveFile* waveFile);

END_SCOPE
