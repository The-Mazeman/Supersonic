#include "header.h"
#include "trackHeader.h"

START_SCOPE(trackHeader)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* trackHeader)
{
	createWindowClass(L"trackHeaderWindowClass", windowCallback);
	createChildWindow(L"trackHeaderWindowClass", window, trackHeader);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	int width, height;
	getWindowDimension(window, &width, &height);
	RECT rectangle = {0, 0, width, height};
	rectangleFrame(deviceContext, &rectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(HWND window, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = globalState.trackHeight;
	resizeWindow(window, width, height);
}
void initialize(HWND window)
{
	HWND textBox;
	textbox::create(window, &textBox);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
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
		case WM_RESIZE:
		{
			handleResize(window, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);

}
END_SCOPE
