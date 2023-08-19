#include "header.h"
#include "ruler.h"

START_SCOPE(ruler)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* child)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);

	state->x = globalState.sidebarWidth;
	state->y = globalState.topbarHeight;
	state->height = globalState.rulerHeight;

	createWindowClass(L"rulerWindowClass", windowCallback);
	createChildWindow(L"rulerWindowClass", parent, child, state);
}
void paintWindow(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	int left = invalidRectangle->left;
	int right = invalidRectangle->right;
	int height = state->height;
	MoveToEx(deviceContext, left, height - 1, 0);
	LineTo(deviceContext, right, height - 1);

    int offsetX = globalState.offsetX;
    invalidRectangle->bottom = height / 2;
	drawGrid(deviceContext, invalidRectangle, offsetX);
	drawMarking(deviceContext, invalidRectangle, offsetX);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, WPARAM wParam)
{
    int x = state->x;
    int y = state->y;

	POINT* parent = (POINT*)wParam;
	int width = parent->x - state->x;
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
