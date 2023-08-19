#include "header.h"
#include "topbar.h"

START_SCOPE(topbar)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* child)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->height = globalState.topbarHeight;

	createWindowClass(L"topBarWindowClass", windowCallback);
	createChildWindow(L"topBarWindowClass", parent, child, state);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, WPARAM wParam)
{
    int x = 0;
    int y = 0;

	POINT* parent = (POINT*)wParam;
	int width = parent->x;
	int height = state->height;

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
			handleResize(state, window, wParam);
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
