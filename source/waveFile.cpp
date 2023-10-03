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
void convertMono16ToStereoFloat(WaveFile* waveFile, float* sampleChunk)
{
	float scaleFactor = 1.0f / 32768.0f;
	__m256 scaler = _mm256_broadcast_ss(&scaleFactor);

	__m256* newSampleChunk = (__m256*)sampleChunk;
	__m64* oldSampleChunk = (__m64*)waveFile->sampleChunk;

	uint framesPerAVX2 = 32 / 8;
	uint64 frameCount = waveFile->frameCount;
	uint64 iterationCount = frameCount / framesPerAVX2;
	for (uint64 i = 0; i != iterationCount; ++i)
	{
		__m128i sampleChunkStereo = _mm_loadl_epi64((__m128i*)oldSampleChunk);
		sampleChunkStereo = _mm_unpacklo_epi16(sampleChunkStereo, sampleChunkStereo);
		__m256i sampleChunkInt32 = _mm256_cvtepi16_epi32(sampleChunkStereo);
		__m256 sampleChunkFloat = _mm256_cvtepi32_ps(sampleChunkInt32);
		*newSampleChunk = _mm256_mul_ps(sampleChunkFloat, scaler);

		++oldSampleChunk;
		++newSampleChunk;
	}
}
void convertStereo16ToStereoFloat(WaveFile* waveFile, float* sampleChunk)
{
	float scaleFactor = 1.0f / 32768.0f;
	__m256 scaler = _mm256_broadcast_ss(&scaleFactor);

	__m256* newSampleChunk = (__m256*)sampleChunk;
	__m128i* oldSampleChunk = (__m128i*)waveFile->sampleChunk;

	uint64 frameCount = waveFile->frameCount;
	uint framesPerAVX2= 32 / 8;
	uint64 iterationCount = frameCount / framesPerAVX2;
	for(uint64 i = 0; i != iterationCount; ++i)
	{
		__m256i sampleChunkInt32 = _mm256_cvtepi16_epi32(*oldSampleChunk);
		__m256 sampleChunkFloat = _mm256_cvtepi32_ps(sampleChunkInt32);
		*newSampleChunk = _mm256_mul_ps(sampleChunkFloat, scaler);

		++oldSampleChunk;
		++newSampleChunk;
	}
}
void writeFloat32Header(Header* header)
{
	header->type = 3;
	header->byteRate = 8u * 48000u;
	header->bitDepth = 32;
	header->channelCount = 2;
	header->blockAlign = header->channelCount * 4u;
}
void szudzikHash(uint a, uint b, uint* hash)
{
	if(a >= b)
	{
		*hash = a * a + a + b;
	}
	else
	{
		*hash = a + b * b;
	}
}
void create(WCHAR* filePath, WaveFile* waveFile, char** waveFilePointer)
{
	loadFile(filePath, waveFilePointer);
	getFileName(filePath, waveFile);
	parseWaveFile(*waveFilePointer, waveFile);
}
void createAudioClip(WaveFile* waveFile, AudioClip** audioClip)
{
	uint channelCount = waveFile->header.channelCount;
	uint frameSize = sizeof(float) * channelCount;
	uint64 frameCount = waveFile->frameCount;
	uint64 sampleChunkSize = frameSize * frameCount;

	uint64 audioClipSize = sampleChunkSize + sizeof(AudioClip);
	void* audioClipPointer = {};
	allocateSmallMemory(audioClipSize, &audioClipPointer);

	AudioClip* newClip = (AudioClip*)audioClipPointer;
	newClip->waveFile = *waveFile;
	float* sampleChunk = (float*)((char*)audioClipPointer + sizeof(AudioClip));
	newClip->waveFile.sampleChunk = sampleChunk;

	uint hash = {};
	uint bitDepth = waveFile->header.bitDepth;
	szudzikHash(bitDepth, channelCount, &hash);
	switch (hash)
	{
		case 273:
		{
			convertMono16ToStereoFloat(waveFile, sampleChunk);
			break;
		}
		case 274:
		{
			convertStereo16ToStereoFloat(waveFile, sampleChunk);
			break;
		}
	}
	*audioClip = newClip;
}
END_SCOPE
