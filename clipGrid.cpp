#include "header.h"
#include "platform.h"
#include "audioClip.h"
#include "clipGrid.h"
#include "waveform.h"
#include "timelineCursor.h"
#include "globalState.h"

START_SCOPE(clipGrid)

void createTimelineCursor(HWND window)
{
	createWindowClass(L"timelineCursorWindowClass", timelineCursor::windowCallback);
	HWND timelineCursor;
	createLayer(L"timelineCursorWindowClass", window, &timelineCursor);
	State* state = (State*)GetProp(window, L"state");
	state->timelineCursor = timelineCursor;
}
void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	SetProp(window, L"state", state);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);
	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_GREY);
	int height = invalidRectangle->bottom;
	drawGrid(deviceContext, invalidRectangle, height);

	EndPaint(window, &paintStruct);
}
void handleSizeChanged(HWND window, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	HWND timelineCursor = state->timelineCursor;
	SendMessage(timelineCursor, WM_RESIZE, 0, lParam);
}
void setTimer(HWND window, WPARAM wParam)
{
	State* state = (State*)GetProp(window, L"state");
	HWND timelineCursor = state->timelineCursor;
	SendMessage(timelineCursor, WM_SETTIMER, wParam, 0);
}
void handleFileDrop(HWND window, WPARAM wParam, LPARAM lParam)
{
	AudioClip* audioClip = (AudioClip*)wParam;
	int trackNumber = (int)lParam;
	audioClip::create(window, audioClip, trackNumber);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
			createWindowClass(L"audioClipWindowClass", audioClip::windowCallback);
			createWindowClass(L"int16WaveformWindowClass", waveform::int16::windowCallback);
			createTimelineCursor(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_SIZE:
		{
			handleSizeChanged(window, lParam);
			break;
		}
		case WM_RESIZE:
		{
			resizeWindow(window, lParam);
			break;
		}
		case WM_HORIZONTALMOUSEWHEEL:
		{
			horizontalScroll(window, lParam);
			break;
		}
		case WM_VERTICALMOUSEWHEEL:
		{
			handleVerticalScroll(window, lParam);
			break;
		}
		case WM_PINCHZOOM:
		{
			zoomWindow(window, lParam);
			break;
		}
		case WM_SETTIMER:
		{
			setTimer(window, wParam);
			break;
		}
		case WM_FILEDROP:
		{
			handleFileDrop(window, wParam, lParam);
			break;
		}
		case WM_NCHITTEST:
		{
			return HTTRANSPARENT;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
END_SCOPE
