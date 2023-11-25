#include "include.hpp"
#include "waveformWindow.hpp"

START_SCOPE(waveformWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void getMaximumAVX2Frame(float** sampleChunk, uint64 frameCount, __m256* max)
{
	uint framesPerIteration = 4;
	uint64 iterationCount = frameCount / framesPerIteration;
	__m256* sample = (__m256*)*sampleChunk;
	__m256 maxAVX2Frame = {};

	for (uint64 i = 0; i != iterationCount; ++i)
	{
		maxAVX2Frame = _mm256_max_ps(*sample, maxAVX2Frame);
		++sample;
	}
	*sampleChunk = (float*)sample;
    *max = maxAVX2Frame;
}
void getMaximumFrame(float** sampleChunk, uint64 frameCount, float* maxFrame)
{
	__m256 max = {};
	float sumLeft = {};
	float sumRight = {};
	getMaximumAVX2Frame(sampleChunk, frameCount, &max);
	for(uint i = 0; i != 4; ++i)
	{
		sumLeft += max.m256_f32[i * 2];
		sumRight += max.m256_f32[(i * 2) + 1];
	}
	sumLeft /= 4.0f;
	sumRight /= 4.0f;
	maxFrame[0] = sumLeft;
	maxFrame[1] = sumRight;
}
DWORD WINAPI createWaveformChunk(LPVOID parameter)
{
    CreateWaveformInfo* createWaveformInfo = (CreateWaveformInfo*)parameter;
    float* sampleChunk = createWaveformInfo->sampleChunk;
    float* waveformSampleChunk = createWaveformInfo->waveformSampleChunk;
    uint framesPerPixel = createWaveformInfo->framesPerPixel;
    uint width = createWaveformInfo->width;

    for(uint i = 0; i != width; ++i)
    {
        float maxFrame[2] = {};
        getMaximumFrame(&sampleChunk, framesPerPixel, (float*)&maxFrame);

        waveformSampleChunk[i * 2] = maxFrame[0];
        waveformSampleChunk[(i * 2) + 1] = maxFrame[1];
    }
    waveformSampleChunk[width * 2] = 0; 
    waveformSampleChunk[(width * 2) + 1] = 0;

    HWND window = createWaveformInfo->window;
    ShowWindow(window, SW_SHOW);
    freeMemory(createWaveformInfo);

    return 0;
}
void create(HWND parent, RECT* boundingBox, float* sampleChunk, uint64 frameCount)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->sampleChunk = sampleChunk;
    state->frameCount = frameCount;
    state->waveformFrameCount = (uint)boundingBox->right;

    HWND window;
    createWindowClass(L"waveformWindowClass", windowCallback);
    createChildWindow(L"waveformWindowClass", parent, state, &window);
    ShowWindow(window, SW_HIDE);
     
    int x = boundingBox->left;
    int y = boundingBox->top;
    int width = boundingBox->right;
    int height = boundingBox->bottom;

    uint waveformChunkSize = (width + 1) * sizeof(float) * 2;

    float* waveformSampleChunk = {};
    allocateMemory(waveformChunkSize, (void**)&waveformSampleChunk);
    state->waveformSampleChunk = waveformSampleChunk;

    int framesPerPixel = (int)(frameCount / (uint)width);

    CreateWaveformInfo* createWaveformInfo = {};
    allocateMemory(sizeof(CreateWaveformInfo), (void**)&createWaveformInfo);
    createWaveformInfo->sampleChunk = sampleChunk;
    createWaveformInfo->framesPerPixel = (uint)framesPerPixel;
    createWaveformInfo->width = (uint)width;
    createWaveformInfo->waveformSampleChunk = waveformSampleChunk;
    createWaveformInfo->window = window;

    createThread(createWaveformChunk, createWaveformInfo);
}
void linearInterpolation(float x0, float x1, float proportion, float* value)
{
    *value = (x0 * (1.0f - proportion)) + (x1 * (proportion));
}
void paint(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

    int width, height;
    getWindowDimension(window, &width, &height);
    int centerY = height / 2;

    uint waveformFrameCount = state->waveformFrameCount;
    float stretchFactor = (float)waveformFrameCount / (float)width;

    float* waveformSampleChunk = state->waveformSampleChunk;
    int left = invalidRectangle->left;
    int right = invalidRectangle->right;
    for(int i = left; i != right; ++i)
    {
        float sampleIndex = (float)i * stretchFactor;
        int indexInteger = (int)sampleIndex;
        float proportion = sampleIndex - (float)indexInteger;

        float x0 = waveformSampleChunk[indexInteger * 2];
        float x1 = waveformSampleChunk[(indexInteger + 1) * 2];

        float sampleValue = {};
        linearInterpolation(x0, x1, proportion, &sampleValue);

        int sampleHeight = (int)(sampleValue * (float)centerY * 1.7f);
        POINT start = {i, centerY + sampleHeight};
        POINT end = {i, centerY - sampleHeight};
        drawLine(deviceContext, &start, &end);
    }

	EndPaint(window, &paintStruct);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_PAINT:
        {
            paint(state, window);
            break;
        }
        case WM_NCHITTEST:
        {
            return HTTRANSPARENT;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
