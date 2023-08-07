#include "header.h"
#include "sidebar.h"

START_SCOPE(sidebar)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* child)
{
	createWindowClass(L"sidebarWindowClass", windowCallback);
	createChildWindow(L"sidebarWindowClass", parent, child);
}
void handleResize(State* state, HWND window, WPARAM wParam)
{
    int x = state->x;
    int y = state->y;

	POINT* parent = (POINT*)wParam;
	int width = state->width;
	int height = parent->y - state->y;

    placeWindow(window, x, y, width, height);
}
void paintWindow(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

    int top = invalidRectangle->top;
    int bottom = invalidRectangle->bottom;
    int width = state->width - 1;
    MoveToEx(deviceContext, width, top, 0);
    LineTo(deviceContext, width, bottom);

	EndPaint(window, &paintStruct);
}
void createTrack(State* state, HWND window, WPARAM wParam)
{
	HWND* trackHeaderArray = state->trackHeaderArray;
    uint trackNumber = (uint)wParam;

    HWND trackHeader = {};
    trackHeader::create(window, &trackHeader);

	int height = globalState.trackHeight;
	int width = globalState.sidebarWidth;
	int y = (int)trackNumber * height;
	placeWindow(trackHeader, 0, y, width, height);

    trackHeaderArray[trackNumber] = trackHeader;
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	state->x = 0;
	state->y = globalState.topbarHeight + globalState.rulerHeight;
	state->width = globalState.sidebarWidth;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
        case WM_CREATE:
        {
            initialize(window);
            break;
        }
		case WM_CREATETRACK:
        {
            createTrack(state, window, wParam);
            break;
        }
		case WM_PAINT:
		{
			paintWindow(state, window);
			break;
		}
		case WM_SIZE:
		{
			//handleWindowSizeChanged(window, lParam);
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

