#include "header.h"
#include "platform.h"
#include "wasapi.h"
#include "timelineCursor.h"
#include "globalState.h"

START_SCOPE(timelineCursor)

void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(HWND window, LPARAM lParam)
{
	int width = 1;
	int height = HIWORD(lParam);
	resizeWindow(window, width, height);
}
void handleTimer(HWND window, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	
	sint64 timeElapsedInMilliseconds = lParam;
	sint64 sampleRateInMilliseconds = globalState.sampleRate / 1000;
	sint64 framePosition = timeElapsedInMilliseconds * sampleRateInMilliseconds;
	sint64 framesPerPixel = globalState.framesPerPixel;
	int pixelsElapsed = (int)(framePosition / framesPerPixel);

	int x = state->x;
	x += pixelsElapsed;

	int width, height;
	getWindowDimension(window, &width, &height);
	MoveWindow(window, x, 0, width, height, 1);
}
void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	SetProp(window, L"state", state);
}
void setCursorPosition(HWND window)
{
	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	State* state = (State*)GetProp(window, L"state");
	state->x = x;
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
		case WM_PLAY:
		{
			setCursorPosition(window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(window, lParam);
			break;
		}
		case WM_TIMER:
		{
			handleTimer(window, lParam);
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