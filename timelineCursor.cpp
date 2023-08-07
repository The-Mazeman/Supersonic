#include "header.h"
#include "timelineCursor.h"

START_SCOPE(timelineCursor)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* timelineCursor)
{
	createWindowClass(L"timelineCursorWindowClass", windowCallback);
	createChildWindow(L"timelineCursorWindowClass", parent, timelineCursor);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, LPARAM lParam)
{
    int x = state->x;
    int y = state->y;
	int width = 1;
	int height = HIWORD(lParam);
	placeWindow(window, x, y, width, height);
}
void handleTimer(State* state, HWND window, WPARAM wParam)
{
	uint64 timeElapsedInMilliseconds = (uint)wParam;
	uint64 sampleRateInMilliseconds = (uint)globalState.sampleRate / 1000;
	uint64 framePosition = timeElapsedInMilliseconds * sampleRateInMilliseconds;
	uint64 framesPerPixel = (uint)globalState.framesPerPixel;
	int pixelsElapsed = (int)(framePosition / framesPerPixel);

	int x = state->x;
	x += pixelsElapsed;

	int width, height;
	getWindowDimension(window, &width, &height);
	placeWindow(window, x, 0, width, height);
}
void setCursorPosition(State* state, HWND window)
{
	int x, y;
	getWindowPosition(window, &x, &y);
	mapToParent(window, &x, &y);

	state->x = x;
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	state->x = 0;
	state->y = 0;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
			initialize(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_PLAY:
		{
			setCursorPosition(state, window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(state, window, lParam);
			break;
		}
		case WM_TIMER:
		{
			handleTimer(state, window, wParam);
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
