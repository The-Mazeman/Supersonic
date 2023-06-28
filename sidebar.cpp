#include "header.h"
#include "platform.h"
#include "sidebar.h"
#include "trackHeaderGroup.h"
#include "globalState.h"

START_SCOPE(sidebar)

void createTrackHeaderGroup(HWND window)
{
	createWindowClass(L"trackHeaderGroupWindowClass", trackHeaderGroup::windowCallback);
	createLayer(L"trackHeaderGroupWindowClass", window, 0);
}
void handleResize(HWND window, WPARAM wParam)
{
	POINT* parent = (POINT*)wParam;
	int width = globalState.sidebarWidth;
	int offsetY = globalState.topbarHeight + globalState.rulerHeight;
	int height = parent->y - offsetY;

	resizeWindow(window, width, height);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createTrackHeaderGroup(window);
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

