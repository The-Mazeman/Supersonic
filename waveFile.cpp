#include "header.h"
#include "waveFile.h"

START_SCOPE(waveFile)

void getFileName(WCHAR* filePath, WaveFile* waveFile)
{
	while (*filePath != 0)
	{
		++filePath;
	}

	uint count = {};
	while (*filePath != 47 && *filePath != 92)
	{
		++count;
		--filePath;
	}
	++filePath;

	uint stringSize = count * sizeof(WCHAR);
	WCHAR* fileName = {};
	allocateSmallMemory(stringSize, (void**)&fileName);
	memcpy(fileName, filePath, stringSize);

	waveFile->name.string = fileName;
	waveFile->name.characterCount = count;
}
void parseWaveFile(char* waveFilePointer, WaveFile* waveFile)
{
	while (1)
	{
		uint tagValue;
		memcpy(&tagValue, waveFilePointer, 4);
		switch (tagValue)
		{
			case 1179011410: // Riff tag value 
			{
				waveFilePointer += 8;
				break;
			}
			case 1163280727: // Wave tag value
			{
				waveFilePointer += 4;
				break;
			}
			case 544501094: // Format tag value
			{
				uint formatChunkSize;
				memcpy(&formatChunkSize, waveFilePointer + 4, 4);
				memcpy(waveFile, (waveFilePointer + 8), 16);
				waveFilePointer += formatChunkSize + 8ull;
				break;
			}
			case 1635017060: // Data tag value
			{
				uint dataChunkSize;
				memcpy(&dataChunkSize, waveFilePointer + 4, 4);
				waveFile->sampleChunk = (float*)(waveFilePointer + 8);
				waveFile->frameCount = dataChunkSize / waveFile->header.blockAlign;
				return;
			}
			default:
			{
				uint unknownChunkSize;
				memcpy(&unknownChunkSize, waveFilePointer + 4, 4);
				waveFilePointer += unknownChunkSize + 8ull;
			}
		}
	}
}
void convertInt16ToFloat(WaveFile* waveFile)
{
	float scaleFactor = 1.0f / 32768.0f;
	__m256 scaler = _mm256_broadcast_ss(&scaleFactor);

	uint64 frameCount = waveFile->frameCount;
	uint64 sampleChunkSize = frameCount * sizeof(float) * 2;
	__m256* newSampleChunk = {};
	allocateSmallMemory(sampleChunkSize, (void**)&newSampleChunk);

	uint framesPerIteration = 4;
	uint64 iterationCount = frameCount / framesPerIteration;
	__m128i* sampleChunk = (__m128i*)waveFile->sampleChunk;
	waveFile->sampleChunk = (float*)newSampleChunk;
	for(uint64 i = 0; i != iterationCount; ++i)
	{
		__m256i sampleChunkInt32 = _mm256_cvtepi16_epi32(*sampleChunk);
		__m256 sampleChunkFloat = _mm256_cvtepi32_ps(sampleChunkInt32);
		*newSampleChunk = _mm256_mul_ps(sampleChunkFloat, scaler);

		++sampleChunk;
		++newSampleChunk;
	}
}
void writeFloat32Header(Header* header)
{
	header->type = 3;
	header->byteRate = 8u * 48000u;
	header->blockAlign = header->channelCount * 4u;
	header->bitDepth = 32;
}
void create(WCHAR* filePath, WaveFile* waveFile)
{
	char* waveFilePointer;
	loadFile(filePath, &waveFilePointer);
	getFileName(filePath, waveFile);

	parseWaveFile(waveFilePointer, waveFile);
	switch(waveFile->header.bitDepth)
	{
		case 16:
		{
			convertInt16ToFloat(waveFile);
		}
	}
	writeFloat32Header(&waveFile->header);
	freeBigMemory(waveFilePointer);
}
END_SCOPE
