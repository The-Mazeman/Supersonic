#include "header.h"
#include "platform.h"
#include "audioClip.h"
#include "waveform.h"
#include "globalState.h"

START_SCOPE(audioClip)

void create(HWND window, AudioClip* audioClip, int trackNumber)
{
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 frameCount = audioClip->waveFile.frameCount;
	int width = (int)(frameCount / (uint64)framesPerPixel);
	audioClip->width = width;

	int height = globalState.trackHeight;
	int y = trackNumber * height;
	int x = audioClip->x;

	HWND audioClipWindow;
	createLayer(L"audioClipWindowClass", window, &audioClipWindow);
	int bitDepth = audioClip->waveFile.header.bitDepth;

	SendMessage(audioClipWindow, WM_FILEDROP, (WPARAM)&audioClip->waveFile, bitDepth);
	MoveWindow(audioClipWindow, x, y, width + 2, height, 1);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleAudioClipDrop(HWND window, LPARAM lParam)
{
	AudioClip* audioClip = (AudioClip*)lParam;
	SetProp(window, L"audioClip", audioClip);
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorX = GET_X_LPARAM(lParam);
	//int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if (cursorX < a + 4)
	{
		return HTLEFT;
	}
	if (cursorX > c - 4)
	{
		return HTRIGHT;
	}
	return HTCAPTION;
}
void handleMoving(HWND window, LPARAM lParam)
{
	RECT* windowRectangle = (RECT*)lParam;
	int x, y;
	getWindowPosition(window, &x, &y);
	windowRectangle->top = y;
	windowRectangle->bottom = y + globalState.trackHeight;

}
void handleFileDrop(HWND window, WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
		case 16:
		{
			WaveFile* waveFile = (WaveFile*)wParam;
			short* sampleChunk = (short*)waveFile->sampleChunk;
			waveform::create(window, waveFile, sampleChunk);
		}
	}
}
void handleSize(HWND window, LPARAM lParam)
{
	HWND child = GetWindow(window, GW_CHILD);
	SendMessage(child, WM_RESIZE, 0, lParam);
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

	int startOffsetDelta = newRectangle->right - oldRectangle->right;
	int endOffsetDelta = oldRectangle->left -  newRectangle->left;


	if (newRectangle->right == oldRectangle->right)
	{
		result = WVR_ALIGNRIGHT;
	}
	return result;
}
void handleSizing(WPARAM wParam, LPARAM lParam)
{
	if(wParam == WMSZ_LEFT)
	{
	}
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_FILEDROP:
		{
			handleFileDrop(window, wParam, lParam);
			break;
		}
		case WM_MOVING:
		{
			handleMoving(window, lParam);
			break;
		}
		case WM_NCCALCSIZE:
		{
			return handleClientPreservation(wParam, lParam);
		}
		case WM_NCHITTEST:
		{
			return handleHitTesting(window, lParam);
		}
		case WM_SIZING:
		{
			handleSizing(wParam, lParam);
		}
		case WM_SIZE:
		{
			handleSize(window, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
