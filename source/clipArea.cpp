#include "header.h"
#include "clipArea.h"

START_SCOPE(clipArea)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void getCursor(State* state, HWND* window)
{
    *window = state->timelineCursor;
}
void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);

	state->x = globalState.sidebarWidth;
	state->y = globalState.topbarHeight + globalState.rulerHeight;

	createWindowClass(L"clipAreaWindowClass", windowCallback);
	createChildWindow(L"clipAreaWindowClass", parent, window, state);

	timelineCursor::create(*window, &state->timelineCursor);
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
void handleFileDrop(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
	uint trackNumber = (uint)lParam;
	AudioClip* audioClip = (AudioClip*)wParam;
	HWND audioClipWindow;
	audioClip::create(window, &audioClipWindow, audioClip);

	int trackHeight = globalState.trackHeight;
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 frameCount = audioClip->waveFile.frameCount;
	int width = (int)(frameCount / (uint64)framesPerPixel);
	int x = state->dropX;
	int y = trackHeight * (int)trackNumber;
	placeWindow(audioClipWindow, x, y, width, trackHeight);
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
void setDropLocation(State* state, WPARAM wParam)
{
	int dropX = (int)wParam;
	state->dropX = dropX;
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
        case WM_SETDROP:
		{
			setDropLocation(state, wParam);
			break;
		}
        case WM_FILEDROP:
        {
            handleFileDrop(state, window, wParam, lParam);
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
