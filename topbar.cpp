#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "topbar.h"

START_SCOPE(topbar)

void create(HWND parent, HWND* child)
{
	createWindowClass(L"topBarWindowClass", topbar::windowCallback);
	createLayer(L"topBarWindowClass", parent, child);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
void handleResize(HWND window, WPARAM wParam)
{
	POINT* parent = (POINT*)wParam;
	int width = parent->x;
	int height = globalState.topbarHeight;

	resizeWindow(window, width, height);
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
