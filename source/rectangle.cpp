#include "header.h"
#include "rectangle.h"

START_SCOPE(rectangle)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, WindowPosition* position)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->position = position;
	createWindowClass(L"rectangleWindowClass", windowCallback);
	createChildWindow(L"rectangleWindowClass", parent, window, state);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window)
{
	WindowPosition* position = state->position;
	int x = position->x;
	int y = position->y;
	int width = position->width;
	int height = position->height;

	placeWindow(window, x, y, width, height);
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
			paintWindow(window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(state, window);
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