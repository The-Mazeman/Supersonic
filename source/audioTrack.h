#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"


START_SCOPE(audioTrack)

struct State
{	Loader loader;
	AudioClip clipList[4];
	uint clipCount;
	uint effectCount;
	TrackControl trackControl;
	AudioEffect audioEffectList[4];
};

void create(HWND window, HWND* audioTrack);

END_SCOPE
