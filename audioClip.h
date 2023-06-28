#pragma once
#include "header.h"
#include "platform.h"
#include "waveFile.h"

struct AudioClip
{
	uint64 startFrame;
	uint64 endFrame;
	uint startOffset;
	uint endOffset;

	int x;
	int width;
	void* start;
	uint64 frameCount;
	WaveFile waveFile;
};

START_SCOPE(audioClip)

void create(HWND window, AudioClip* audioClip, int trackNumber);
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE
