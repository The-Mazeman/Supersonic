#include "header.h"
#include "waveform.h"

START_SCOPE(waveform)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

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

void getMaximumAVX2Frame(float** waveFileFramePointer, uint64 frameCount, __m256* max)
{
	uint framesPerIteration = 4;
	uint64 iterationCount = frameCount / framesPerIteration;
	__m256* sample = (__m256*) * waveFileFramePointer;
	for (uint64 i = 0; i != iterationCount; ++i)
	{
		*max = _mm256_max_ps(*sample, *max);
		++sample;
	}
	*waveFileFramePointer = (float*)sample;
}
void getMaximumFrame(float** waveFileFramePointer, uint64 frameCount, float* maxFrame)
{
	__m256 max = {};
	float sumLeft = {};
	float sumRight = {};
	getMaximumAVX2Frame(waveFileFramePointer, frameCount, &max);
	for(uint i = 0; i != 4; ++i)
	{
		sumLeft += max.m256_f32[i];
		sumRight += max.m256_f32[i + 1];
	}
	sumLeft /= 4;
	sumRight /= 4;
	maxFrame[0] = sumLeft;
	maxFrame[1] = sumRight;

}
void drawWaveformAverage(HDC deviceContext, RECT* invalidRectangle, float* waveform)
{
	int x0 = invalidRectangle->left;
	int x1 = invalidRectangle->right;

	int height = globalState.trackHeight;
	int centerY = height / 2;

	sint64 framesPerPixel = globalState.framesPerPixel / FRAMES_TO_AVERAGE;
	sint64 frameOffset = x0 * framesPerPixel;
	waveform += frameOffset * 2;

	float scaleFactor = (float)centerY;

	for (int i = x0; i != x1; ++i)
	{
		float maxFrame[2] = {};
		getMaximumFrame(&waveform, (uint64)framesPerPixel, maxFrame);

		int sampleHeight = (int)(maxFrame[0] / scaleFactor);
		drawMirroredSample(deviceContext, i, centerY, sampleHeight);
	}
}
void drawWaveform(HDC deviceContext, RECT* invalidRectangle, float* waveFile)
{
	int x0 = invalidRectangle->left;
	int x1 = invalidRectangle->right;

	int height = globalState.trackHeight;
	int centerY = height / 2;
	float scaleFactor = (float)centerY;

	sint64 framesPerPixel = globalState.framesPerPixel;
	sint64 frameOffset = x0 * framesPerPixel;
	waveFile += frameOffset * 2;

	for (int i = x0; i != x1; ++i)
	{
		float maxFrame[2] = {};
		getMaximumFrame(&waveFile, (uint64)framesPerPixel, maxFrame);

		int sampleHeight = (int)(maxFrame[0] * scaleFactor);
		drawMirroredSample(deviceContext, i, centerY, sampleHeight);
	}
}
void handleResize(HWND window, LPARAM lParam)
{
	int width = LOWORD(lParam) - 2;
	int height = HIWORD(lParam) - 2;
	MoveWindow(window, 1, 1, width, height, 1);
}
void paintWindow(State* state, HWND window)
{
	if(state->waveFile.sampleChunk == 0)
	{
		return;
	}

	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);
	sint64 framesPerPixel = globalState.framesPerPixel;

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

	if (framesPerPixel > FRAMES_TO_AVERAGE)
	{
		float* waveform = state->waveform;
		drawWaveformAverage(deviceContext, invalidRectangle, waveform);
	}
	else
	{
		float* sampleChunk = state->waveFile.sampleChunk;
		drawWaveform(deviceContext, invalidRectangle, sampleChunk);
	}

	EndPaint(window, &paintStruct);
}
LRESULT handleClientPreservation(WPARAM wParam, LPARAM lParam)
{
	if (!wParam)
	{
		return 0;
	}
	NCCALCSIZE_PARAMS* parameter = (NCCALCSIZE_PARAMS*)lParam;
	RECT* newRectangle = parameter->rgrc;
	RECT* oldRectangle = newRectangle + 1;
	LRESULT result = {};
	if (newRectangle->right == oldRectangle->right)
	{

		//int delta =  newRectangle->left - oldRectangle->left;
		//state.startOffset += delta;

		result = WVR_ALIGNRIGHT;
	}
	return result;
}
void reduceFrameCount(float* waveFile, float* waveform, uint64 waveformFrameCount)
{
	float maxFrame[2] = {};
	for (uint64 i = 0; i != waveformFrameCount; ++i)
	{
		getMaximumFrame(&waveFile, FRAMES_TO_AVERAGE, maxFrame);
		*waveform = maxFrame[0];
		++waveform;
		*waveform = maxFrame[1];
		++waveform;
	}
}
void createWaveformData(State* state)
{
	uint64 frameCount = state->waveFile.frameCount;
	uint64 reducedFrameCount = (frameCount / FRAMES_TO_AVERAGE);

	uint channelCount = state->waveFile.header.channelCount;
	uint waveformFrameSize = sizeof(float) * channelCount;
	uint64 waveformChunkSize = reducedFrameCount * waveformFrameSize;

	float* waveformChunk = {};
	allocateSmallMemory(waveformChunkSize, (void**)&waveformChunk);
	state->waveform = waveformChunk;

	float* sampleChunk = state->waveFile.sampleChunk;
	reduceFrameCount(sampleChunk, waveformChunk, reducedFrameCount);
}
void create(HWND window, HWND* waveform, WaveFile* waveFile)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->waveFile = *waveFile;

	createWaveformData(state);
	createWindowClass(L"waveformWindowClass", windowCallback);
	createChildWindow(L"waveformWindowClass", window, waveform, state);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
			setState(window, lParam);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(state, window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(window, lParam);
			break;
		}
		case WM_NCCALCSIZE:
		{
			return handleClientPreservation(wParam, lParam);
		}
		case WM_NCHITTEST:
		{
			return HTTRANSPARENT;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE


