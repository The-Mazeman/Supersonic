#include "header.h"
#include "platform.h"
#include "rulerGrid.h"
#include "globalState.h"

START_SCOPE(ruler)

void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_RED);

	EndPaint(window, &paintStruct);
}
void createGridWindow(HWND window)
{
	createWindowClass(L"rulerGridWindowClass", rulerGrid::windowCallback);
	createLayer(L"rulerGridWindowClass", window, 0);
}
void handleResize(HWND window, WPARAM wParam)
{
	POINT* parent = (POINT*)wParam;
	int width = parent->x - globalState.sidebarWidth;
	int height = globalState.rulerHeight;

	resizeWindow(window, width, height);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createGridWindow(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_SIZE:
		{
			handleWindowSizeChanged(window, lParam);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(window, wParam);
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