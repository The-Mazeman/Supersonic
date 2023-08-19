#include "header.h"
#include "rectangle.h"

START_SCOPE(rectangle)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, WindowPosition* position)
{
	createWindowClass(L"rectangleWindowClass", windowCallback);
	createChildWindow(L"rectangleWindowClass", parent, window, position);
	ShowWindow(*window, SW_HIDE);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(WindowPosition* position, HWND window)
{
	int x = position->x;
	int y = position->y;
	int width = position->width;
	int height = position->height;

	placeWindow(window, x, y, width, height);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	WindowPosition* position = (WindowPosition*)GetProp(window, L"state");
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
			handleResize(position, window);
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