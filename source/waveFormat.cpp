#include "include.hpp"
#include "waveFormat.hpp"

START_SCOPE(waveFormat)
    
void parseWaveFile(void* waveFilePointer, WaveFile* waveFile)
{
	while (1)
	{
		uint tagValue;
		memcpy(&tagValue, waveFilePointer, 4);
		switch (tagValue)
		{
			case 1179011410: // Riff tag value 
			{
				waveFilePointer = (char*)waveFilePointer + 8;
				break;
			}
			case 1163280727: // Wave tag value
			{
				waveFilePointer = (char*)waveFilePointer + 4;
				break;
			}
			case 544501094: // Format tag value
			{
				uint formatChunkSize;
				memcpy(&formatChunkSize, (char*)waveFilePointer + 4, 4);
				memcpy(waveFile, (char*)waveFilePointer + 8, 16);
				waveFilePointer = (char*)waveFilePointer + formatChunkSize + 8ull;
				break;
			}
			case 1635017060: // Data tag value
			{
				uint dataChunkSize;
				memcpy(&dataChunkSize, (char*)waveFilePointer + 4, 4);
				waveFile->sampleChunk = (float*)((char*)waveFilePointer + 8);
				waveFile->frameCount = dataChunkSize / waveFile->header.blockAlign;
				return;
			}
			default:
			{
				uint unknownChunkSize;
				memcpy(&unknownChunkSize, (char*)waveFilePointer + 4, 4);
				waveFilePointer = (char*)waveFilePointer + unknownChunkSize + 8ull;
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
	uint framesPerAVX2 = 32 / 8;
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
void create(WCHAR* filePath, float** sampleChunk, uint64* frameCount)
{
    void* waveFilePointer;
    loadFile(filePath, &waveFilePointer);

    WaveFile waveFile;
    parseWaveFile(waveFilePointer, &waveFile);

    uint frameSize = 8;
    *frameCount = waveFile.frameCount;
    uint64 sampleChunkSize = frameSize * (*frameCount);
    allocateMemory(sampleChunkSize, (void**)sampleChunk);

    uint hash = {};
	uint bitDepth = waveFile.header.bitDepth;
	uint channelCount = waveFile.header.channelCount;
	szudzikHash(bitDepth, channelCount, &hash);
	switch (hash)
	{
		case 273:
		{
			convertMono16ToStereoFloat(&waveFile, *sampleChunk);
			break;
		}
		case 274:
		{
			convertStereo16ToStereoFloat(&waveFile, *sampleChunk);
			break;
		}
	}
    freeMemory(waveFilePointer);
}

END_SCOPE
