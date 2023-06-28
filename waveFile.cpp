#include "header.h"
#include "platform.h"
#include "waveFile.h"
#include "globalState.h"

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
	allocateSmallMemory(stringSize, (char**)&fileName);
	memcpy(fileName, filePath, stringSize);

	waveFile->name.string = fileName;
	waveFile->name.characterCount = count;
}
void parseWaveFile(char* waveFilePointer, WaveFile* waveFile, void** sampleChunk)
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
				*sampleChunk = (waveFilePointer + 8);
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
void extractSampleChunk(WaveFile* waveFile, void** sampleChunk)
{
	uint waveFrameSize = waveFile->header.blockAlign;
	uint64 waveFrameCount = waveFile->frameCount;
	uint64 waveSampleChunkSize = waveFrameCount * waveFrameSize;

	char* waveSampleChunk;
	allocateSmallMemory(waveSampleChunkSize, &waveSampleChunk);
	memcpy(waveSampleChunk, *sampleChunk, waveSampleChunkSize);

	*sampleChunk = waveSampleChunk;
}
void load(WCHAR* filePath, WaveFile* waveFile, void** sampleChunk)
{
	char* waveFilePointer;
	loadFile(filePath, &waveFilePointer);
	getFileName(filePath, waveFile);

	parseWaveFile(waveFilePointer, waveFile, sampleChunk);
	extractSampleChunk(waveFile, sampleChunk);
	freeBigMemory(waveFilePointer);
}
END_SCOPE