#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "topbar.h"
#include "fader.h"

START_SCOPE(trackControl)

struct State
{
	HWND topbar;
	HWND volumeFader;
	TrackControl* trackControl;
};

void create(HWND parent, HWND* child);

END_SCOPE
