#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "trackHeader.h"
#include "textbox.h"

START_SCOPE(trackHeader)

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
void create(HWND window, String* name, int trackNumber, int width,  HWND* trackHeaderHandle)
{
	HWND trackHeader;
	createLayer(L"trackHeaderWindowClass", window, &trackHeader);
	*trackHeaderHandle = trackHeader;

	int height = globalState.trackHeight;
	int y = trackNumber * height;
	MoveWindow(trackHeader, 0, y, width, height, 1);

	textbox::create(trackHeader, name, width);
	++globalState.trackCount;
}
void handleResize(HWND window, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = globalState.trackHeight;
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
			handleResize(window, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);

}
END_SCOPE
