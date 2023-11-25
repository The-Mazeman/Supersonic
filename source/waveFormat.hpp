#pragma once 

#include "define.hpp"
#include "dataType.hpp"
#include "platformWindows.hpp"

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
	float* sampleChunk;
};

START_SCOPE(waveFormat)

void create(WCHAR* filePath, float** sampleChunk, uint64* frameCount);

END_SCOPE
