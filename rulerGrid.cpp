#include "header.h"
#include "platform.h"
#include "globalState.h"

START_SCOPE(rulerGrid)

void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);
	int height = globalState.rulerHeight;
	drawMarking(deviceContext, invalidRectangle);
	drawGrid(deviceContext, invalidRectangle, height / 2);

	int left = invalidRectangle->left;
	int right = invalidRectangle->right;
	MoveToEx(deviceContext, left, height - 1, 0);
	LineTo(deviceContext, right, height - 1);

	EndPaint(window, &paintStruct);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
		{
			paintWindow(window);
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
		case WM_PINCHZOOM:
		{
			zoomWindow(window, lParam);
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
