#pragma once
#include "header.h"
#include "platform.h"


START_SCOPE(waveFile)

void create(WCHAR*, WaveFile*, char**);
void createAudioClip(WaveFile*, AudioClip**);

END_SCOPE
