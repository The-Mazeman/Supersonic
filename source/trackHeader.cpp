#include "header.h"
#include "trackHeader.h"

START_SCOPE(trackHeader)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, HWND audioTrack)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->audioTrack = audioTrack;

	createWindowClass(L"trackHeaderWindowClass", windowCallback);
	createChildWindow(L"trackHeaderWindowClass", parent, window, state);

	HWND textbox;
	textbox::create(*window, &textbox);
	state->textbox = textbox;
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
void handleMouseLeftClick(State* state)
{
	HWND audioTrack = state->audioTrack;
	SendMessage(audioTrack, WM_SETCONTROL, 0, 0);
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
			paintWindow(window);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(window, lParam);
			break;
		}
		case WM_LBUTTONDOWN:
		{
			handleMouseLeftClick(state);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
END_SCOPE
