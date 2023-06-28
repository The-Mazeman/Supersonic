#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "clipArea.h"
#include "clipGrid.h"

START_SCOPE(clipArea)

void handleResize(HWND window, WPARAM wParam)
{
	POINT* parent = (POINT*)wParam;
	int offsetX = globalState.sidebarWidth;
	int width = parent->x - offsetX;

	int offsetY = globalState.topbarHeight + globalState.rulerHeight;
	int height = parent->y - offsetY;

	resizeWindow(window, width, height);
}
void createGridWindow(HWND window)
{
	createWindowClass(L"clipGridWindowClass", clipGrid::windowCallback);
	createLayer(L"clipGridWindowClass", window, 0);
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
	switch (message)
	{
		case WM_CREATE:
		{
			createGridWindow(window);
			break;
		}
		case WM_PAINT:
		{
			//paintWindow(window);
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