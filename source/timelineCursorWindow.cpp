#include "include.hpp"
#include "timelineCursorWindow.hpp"

START_SCOPE(timelineCursorWindow)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateMemory(sizeof(State), (void**)&state);

	createWindowClass(L"timelineCursorWindowClass", windowCallback);
	createChildWindow(L"timelineCursorWindowClass", parent, state, window);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_RED);

	EndPaint(window, &paintStruct);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_PAINT:
		{
			paintWindow(window);
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

