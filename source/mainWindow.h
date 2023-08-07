#pragma once
#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "topbar.h"
#include "ruler.h"
#include "clipArea.h"
#include "sidebar.h"
#include "waveFile.h"
#include "audioEngine.h"

START_SCOPE(mainWindow)

struct State
{
	HWND topbar;
	HWND ruler;
	HWND sidebar;
	HWND clipArea;
	HWND audioEngine;

	int playing;
	uint trackCount;
};

void create();

END_SCOPE
