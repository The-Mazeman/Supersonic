#include "header.h"
#include "clipArea.h"

START_SCOPE(clipArea)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void getCursor(State* state, HWND* window)
{
    *window = state->timelineCursor;
}
void create(HWND parent, HWND* child)
{
	createWindowClass(L"clipAreaWindowClass", windowCallback);
	createChildWindow(L"clipAreaWindowClass", parent, child);
}
void handleResize(State* state, HWND window, WPARAM wParam)
{
    int x = state->x;
    int y = state->y;

	POINT* parent = (POINT*)wParam;
	int width = parent->x - x;
	int height = parent->y - y;

    placeWindow(window, x, y, width, height);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_GREY);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

    int offsetX = globalState.offsetX;
	drawGrid(deviceContext, invalidRectangle, offsetX);
	EndPaint(window, &paintStruct);
}
void handleFileDrop(HWND window, WPARAM wParam, LPARAM lParam)
{
	uint trackNumber = (uint)lParam;
	HWND audioClipWindow;
	audioClip::create(window, &audioClipWindow);
	SendMessage(audioClipWindow, WM_FILEDROP, wParam, lParam);

	AudioClip* audioClip = (AudioClip*)wParam;
	int trackHeight = globalState.trackHeight;
	int width = audioClip->width;
	int x = audioClip->x;
	int y = trackHeight * (int)trackNumber;
	placeWindow(audioClipWindow, x, y, width, trackHeight);
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);

	state->x = globalState.sidebarWidth;
	state->y = globalState.topbarHeight + globalState.rulerHeight;

    timelineCursor::create(window, &state->timelineCursor);
}
void moveCursor(State* state, WPARAM wParam)
{
	int cursorX = (int)wParam;
	HWND timelineCursor = state->timelineCursor;
	int width, height;
	getWindowDimension(timelineCursor, &width, &height);
	placeWindow(timelineCursor, cursorX, 0, width, height);

	sint64 framesPerPixel = globalState.framesPerPixel;
	uint loadFrameCount = globalState.audioEndpointFrameCount / 2;
	uint64 readCursor = cursorX * (uint64)framesPerPixel;
	readCursor /= loadFrameCount;
	globalState.readCursor = readCursor;
}
void giveCursor(State* state, WPARAM wParam)
{
	HWND* window = (HWND*)wParam;
	*window = state->timelineCursor;
}
void resizeChild(State* state, LPARAM lParam)
{
	HWND timelineCursor = state->timelineCursor;
	SendMessage(timelineCursor, WM_RESIZE, 0, lParam);
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
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_SIZE:
		{
			resizeChild(state, lParam);
			break;
		}
		case WM_GETCURSOR:
		{
			giveCursor(state, wParam);
			break;
		}
		case WM_RESIZE:
		{
			handleResize(state, window, wParam);
			break;
		}
        case WM_FILEDROP:
        {
            handleFileDrop(window, wParam, lParam);
            break;
        }
		case WM_MOVECURSOR:
		{
			moveCursor(state, wParam);
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
