#pragma once
#include "header.h"
#include "platform.h"
#include "audioClip.h"

START_SCOPE(trackHeader)

struct State
{
	String name;
	AudioClip* clipList;
	uint clipCount;
	int padding;
};

void create(HWND window, String* name, int trackNumber, int width, HWND* trackHeaderHandle);
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

END_SCOPE