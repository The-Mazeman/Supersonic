#include "header.h"
#include "platform.h"
#include "waveform.h"
#include "globalState.h"

#define WAVEFORM_BYTE_DEPTH 1
#define WAVEFORM_BIT_DEPTH 8

START_SCOPE(waveform)

void drawMirroredSample(HDC deviceContext, int x, int y, int sampleHeight)
{
	MoveToEx(deviceContext, x, y + sampleHeight, 0);
	LineTo(deviceContext, x, y - sampleHeight);
}
void drawSingleSample(HDC deviceContext, int x, int y, int sampleHeight)
{
	MoveToEx(deviceContext, x, y, 0);
	LineTo(deviceContext, x, y - sampleHeight);
}
void getMaximumAVX2Frame(char** waveFileFramePointer, uint64 frameCount, __m256i* max)
{
	uint framesPerIteration = 32 / 2;
	uint64 iterationCount = frameCount / framesPerIteration;

	__m256i a = {};
	__m256i* framePointer = (__m256i*) * waveFileFramePointer;

	for (uint64 j = 0; j != iterationCount; ++j)
	{
		a = _mm256_load_si256(framePointer);
		*max = _mm256_max_epi8(a, *max);
		++framePointer;
	}
	*waveFileFramePointer = (char*)framePointer;
}
void getMaximumFrame(char** waveFileFramePointer, uint64 frameCount, char* maxFrame)
{
	__m256i max = {};
	getMaximumAVX2Frame(waveFileFramePointer, frameCount, &max);

	__m256i a = {};
	a = _mm256_permute2x128_si256(max, max, 1);
	max = _mm256_max_epi8(a, max);
	a = _mm256_permute4x64_epi64(max, 1);
	max = _mm256_max_epi8(a, max);
	a = _mm256_permutevar8x32_epi32(max, _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 1));
	max = _mm256_max_epi8(a, max);
	a = _mm256_shufflelo_epi16(max, 1);
	max = _mm256_max_epi8(a, max);

	maxFrame[0] = max.m256i_i8[0];
	maxFrame[1] = max.m256i_i8[1];
}
void getMaximumAVX2Frame(short** waveFileFramePointer, uint64 frameCount, __m256i* max)
{
	uint framesPerIteration = 32 / 4;
	uint64 iterationCount = frameCount / framesPerIteration;

	__m256i a = {};
	__m256i* framePointer = (__m256i*) * waveFileFramePointer;
	for (uint64 j = 0; j != iterationCount; ++j)
	{
		a = _mm256_load_si256(framePointer);
		*max = _mm256_max_epi16(a, *max);
		++framePointer;
	}
	*waveFileFramePointer = (short*)framePointer;
}
void getMaximumFrame(short** waveFileFramePointer, uint64 frameCount, short* maxFrame)
{
	__m256i max = {};
	getMaximumAVX2Frame(waveFileFramePointer, frameCount, &max);

	__m256i a = {};
	a = _mm256_permute2x128_si256(max, max, 1);
	max = _mm256_max_epi16(a, max);
	a = _mm256_permute4x64_epi64(max, 1);
	max = _mm256_max_epi16(a, max);
	a = _mm256_permutevar8x32_epi32(max, _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 1));
	max = _mm256_max_epi16(a, max);

	_mm_storeu_si32(maxFrame, _mm256_castsi256_si128(max));
}
void calculateLastFrame(short** waveFilePointer, uint endFrameCount, short* maxFrame)
{
	getMaximumFrame(waveFilePointer, endFrameCount, maxFrame);
	uint leftOverFrameCount = endFrameCount % 8;
	short* waveFramePointer = (short*)*waveFilePointer;
	for (uint i = 0; i != leftOverFrameCount; ++i)
	{
		if (*waveFramePointer > maxFrame[0])
		{
			maxFrame[0] = *waveFramePointer;
		}
		++waveFramePointer;
		if (*waveFramePointer > maxFrame[1])
		{
			maxFrame[1] = *waveFramePointer;
		}
		++waveFramePointer;
	}
}
void reduceFrameCount(short* waveFile, char* waveform, uint64 waveFrameCount, uint64 waveformFrameCount)
{
	uint endBlockFrameCount = waveFrameCount % FRAMES_TO_AVERAGE;
	short maxFrame[2] = {};
	for (uint64 i = 0; i != waveformFrameCount - 1; ++i)
	{
		getMaximumFrame(&waveFile, FRAMES_TO_AVERAGE, maxFrame);
		*waveform = maxFrame[0] >> 8;
		++waveform;
		*waveform = maxFrame[1] >> 8;
		++waveform;
	}
	calculateLastFrame(&waveFile, endBlockFrameCount, maxFrame);
	*waveform = maxFrame[0] >> 8;
	++waveform;
	*waveform = maxFrame[1] >> 8;
	++waveform;
}
void calculateWaveform(WaveFile* waveFile, short* sampleChunk, char** waveformChunk)
{
	uint channelCount = waveFile->header.channelCount;
	uint64 frameCount = waveFile->frameCount;

	uint64 reducedFrameCount = (frameCount / FRAMES_TO_AVERAGE);
	++reducedFrameCount;

	uint waveformFrameSize = (uint)(channelCount * WAVEFORM_BYTE_DEPTH);
	uint64 waveformChunkSize = reducedFrameCount * waveformFrameSize;

	allocateSmallMemory(waveformChunkSize, waveformChunk);

	reduceFrameCount(sampleChunk, *waveformChunk, frameCount, reducedFrameCount);
}
void drawWaveformAverage(HDC deviceContext, RECT* invalidRectangle, char* waveform)
{
	int x0 = invalidRectangle->left;
	int x1 = invalidRectangle->right;

	int height = invalidRectangle->bottom;
	int centerY = height / 2;

	int maximumSampleHeight = centerY;
	int maximumSampleValue = SCHAR_MAX;

	sint64 framesPerPixel = globalState.framesPerPixel / FRAMES_TO_AVERAGE;
	sint64 frameOffset = x0 * framesPerPixel;
	waveform += frameOffset * 2;

	float scaleFactor = (float)maximumSampleHeight / (float)maximumSampleValue;

	for (int i = x0; i != x1; ++i)
	{
		char maxFrame[2] = {};
		getMaximumFrame(&waveform, (uint64)framesPerPixel, maxFrame);

		int sampleHeight = (int)((float)maxFrame[0] / scaleFactor);
		drawMirroredSample(deviceContext, i, centerY, sampleHeight);
	}
}
void drawWaveform(HDC deviceContext, RECT* invalidRectangle, short* wavefile)
{
	int x0 = invalidRectangle->left;
	int x1 = invalidRectangle->right;

	int height = invalidRectangle->bottom;
	int centerY = height / 2;
	int maximumSampleHeight = centerY;
	int maximumSampleValue = SHRT_MAX;
	float scaleFactor = (float)maximumSampleHeight / (float)maximumSampleValue;

	sint64 framesPerPixel = globalState.framesPerPixel;
	sint64 frameOffset = x0 * framesPerPixel;
	wavefile += frameOffset * 2;

	for (int i = x0; i != x1; ++i)
	{
		short maxFrame[2] = {};
		getMaximumFrame(&wavefile, (uint64)framesPerPixel, maxFrame);

		int sampleHeight = (int)((float)maxFrame[0] * scaleFactor);
		drawMirroredSample(deviceContext, i, centerY, sampleHeight);
	}
}
void handleResize(HWND window, LPARAM lParam)
{
	int width = LOWORD(lParam) - 2;
	int height = HIWORD(lParam) - 2;
	MoveWindow(window, 1, 1, width, height, 1);
}
void create(HWND parent, WaveFile* waveFile, short* sampleChunk)
{
	HWND waveformWindow;
	createLayer(L"int16WaveformWindowClass", parent, &waveformWindow);

	char* waveformChunk;
	calculateWaveform(waveFile, sampleChunk, &waveformChunk);

	SetProp(waveformWindow, L"sampleChunk", sampleChunk);
	SetProp(waveformWindow, L"waveformChunk", waveformChunk);
}
START_SCOPE(int16)

void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_RED);

	int width, height;
	getWindowDimension(window, &width, &height);
	invalidRectangle->bottom = height;

	sint64 framesPerPixel = globalState.framesPerPixel;

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);
	State* state = (State*)GetProp(window, L"state");
	int startOffset = state->startOffset;

	if (framesPerPixel > FRAMES_TO_AVERAGE)
	{
		char* waveform = (char*)GetProp(window, L"waveformChunk");
		waveform += startOffset * framesPerPixel * 2;
		drawWaveformAverage(deviceContext, invalidRectangle, waveform);
	}
	else
	{
		short* sampleChunk = (short*)GetProp(window, L"sampleChunk");
		sampleChunk += startOffset * framesPerPixel * 2;
		drawWaveform(deviceContext, invalidRectangle, sampleChunk);
	}

	EndPaint(window, &paintStruct);
}
LRESULT handleClientPreservation(HWND window, WPARAM wParam, LPARAM lParam)
{
	if (!wParam)
	{
		return 0;
	}
	NCCALCSIZE_PARAMS* parameter = (NCCALCSIZE_PARAMS*)lParam;
	RECT* newRectangle = parameter->rgrc;
	RECT* oldRectangle = newRectangle + 1;
	LRESULT result = {};
	State* state = (State*)GetProp(window, L"state");
	if (newRectangle->right == oldRectangle->right)
	{

		int delta =  newRectangle->left - oldRectangle->left;
		state->startOffset += delta;

		result = WVR_ALIGNRIGHT;
	}
	return result;
}
void createState(HWND window)
{
	State* state;
	allocateSmallMemory(sizeof(State), (char**)&state);
	state->startOffset = 0;
	state->endOffset = 0;
	SetProp(window, L"state", state);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(window, lParam);
			break;
		}
		case WM_NCCALCSIZE:
		{
			return handleClientPreservation(window, wParam, lParam);
		}
		case WM_NCHITTEST:
		{
			return HTTRANSPARENT;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE

END_SCOPE

