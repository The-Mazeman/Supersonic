#pragma once
#include "header.h"
#include "platform.h"
#include "waveFile.h"

START_SCOPE(waveform)

struct State
{
	int startOffset;
	int endOffset;
};
void create(HWND parent, WaveFile* waveFile, short* sampleChunk);

START_SCOPE(int16)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE

END_SCOPE