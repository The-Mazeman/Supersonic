#pragma once
#include "header.h"
#include "platform.h"

struct Header
{
	uint16 type;
	uint16 channelCount;
	uint32 sampleRate;
	uint32 byteRate;
	uint16 blockAlign;
	uint16 bitDepth;
};
struct WaveFile
{
	Header header;
	uint64 frameCount;
	void* sampleChunk;
	String name;
};
START_SCOPE(waveFile)

void load(WCHAR*, WaveFile*, void**);

END_SCOPE