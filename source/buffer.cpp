#include "include.hpp"
#include "buffer.hpp"

void startBufferLoader(HWND* trackArray, uint trackCount, BufferLoaderInfo* loaderInfo)
{
    for(uint i = 0; i != trackCount; ++i)
    {
        BufferLoaderInfo* bufferLoaderInfo = {};
        allocateMemory(sizeof(BufferLoaderInfo), (void**)&bufferLoaderInfo);
        *bufferLoaderInfo = *loaderInfo;
        bufferLoaderInfo->loaderPosition = i;
        SendMessage(trackArray[i], WM_STARTOUTPUTLOADER, (WPARAM)bufferLoaderInfo, 0);
    }
}
void fillSample(float* input, float* output, uint iterationCount, uint loaderCount)
{
    __m256* inputAVX2 = (__m256*)input;
	__m256* outputAVX2 = (__m256*)output;
	for(uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)outputAVX2, *inputAVX2);
		outputAVX2 += loaderCount;
		++inputAVX2;
	}
}
DWORD WINAPI bufferLoader(LPVOID parameter)
{
    BufferLoaderInfo* bufferLoaderInfo = (BufferLoaderInfo*)parameter;

    uint bufferOffset = bufferLoaderInfo->loaderPosition * AVX2_FRAME_SIZE;
	uint loaderCount = bufferLoaderInfo->loaderCount;
	float* outputBuffer = (float*)((char*)bufferLoaderInfo->outputBuffer + bufferOffset);

	float* inputBuffer = bufferLoaderInfo->inputBuffer;
	HANDLE finishSemaphore = bufferLoaderInfo->finishSemaphore;

	uint frameCount = bufferLoaderInfo->frameCount;
	uint framesPerAVX2 = 32 / 8;
	uint iterationCount = frameCount / framesPerAVX2;

    HANDLE startEvent = bufferLoaderInfo->startEvent;
    HANDLE exitSemaphore = bufferLoaderInfo->exitSemaphore;
    HANDLE waitHandle[] = {startEvent, exitSemaphore};
    uint running = 1;
    while(running)
    {
        uint signal = WaitForMultipleObjects(2, waitHandle, 0, INFINITE);
        switch(signal)
        {
            case WAIT_OBJECT_0:
            {
				fillSample(inputBuffer, outputBuffer, iterationCount, loaderCount);
				ReleaseSemaphore(finishSemaphore, 1, 0);
                break;
            }
            case WAIT_OBJECT_0 + 1:
            {
                freeMemory(bufferLoaderInfo);
                running = 0;
            }
        }
    }
    return 0;
}
void accumulateFrame(float* input, float* output, uint iterationCount, uint inputLoaderCount)
{
    __m256* inputAVX2 = (__m256*)input;
    __m256* outputAVX2 = (__m256*)output;
	for(uint i = 0; i != iterationCount; ++i)
	{
		__m256 sum = {};
		for(uint j = 0; j != inputLoaderCount; ++j)
		{
			sum = _mm256_add_ps(sum, *inputAVX2);
			++inputAVX2;
		}
		*outputAVX2 = _mm256_load_ps((float*)&sum);
        ++outputAVX2;
	}
}
void copySample(float** input, float* output, uint iterationCount)
{
	__m256* inputAVX2 = (__m256*)*input;
	__m256* outputAVX2 = (__m256*)output;
	for (uint i = 0; i != iterationCount; ++i)
	{
		_mm256_store_ps((float*)outputAVX2, *inputAVX2);
		++outputAVX2;
		++inputAVX2;
	}
	*input = (float*)inputAVX2;
}
