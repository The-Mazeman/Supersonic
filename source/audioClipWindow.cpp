#include "include.hpp"
#include "audioClipWindow.hpp"

START_SCOPE(audioClipWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(AudioClip* audioClip, RECT* boundingBox, HWND parent, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->audioClip = audioClip;

    createWindowClass(L"audioClipWindowClass", windowCallback);
    createChildWindow(L"audioClipWindowClass", parent, state, window);

    int x = boundingBox->left;
    int y = boundingBox->top;
    int width = boundingBox->right;
    int height = boundingBox->bottom;

    boundingBox->left = 1;
    boundingBox->top = 1;
    boundingBox->right -= 2;
    boundingBox->bottom -= 2;

    waveformWindow::create(*window, boundingBox, audioClip->sampleChunk, audioClip->frameCount);
    placeWindow(*window, x, y, width, height);
}
void paint(HWND window)
{
    PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

    EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, LPARAM lParam)
{
    POINT* value = (POINT*)lParam;
    int framesPerPixel  = value->x;
    int offsetX  = value->y;

    int x, y;
    getWindowPosition(window, &x, &y);
    HWND parent = GetAncestor(window, GA_PARENT);
    POINT point = {x, y};
    ScreenToClient(parent, &point);
    int width, height;
    getWindowDimension(window, &width, &height);

    x = (int)(state->audioClip->startFrame / (uint)framesPerPixel);
    x -= offsetX;

    y = point.y;
    width = (int)(state->audioClip->frameCount / (uint64)framesPerPixel); 
    placeWindow(window, x, y, width, height);
}
void handleStartFrameChange(State* state, WPARAM wParam)
{
    uint64 startFrame = (uint64)wParam;
    state->audioClip->startFrame = startFrame;
}
void handleSizeChanged(HWND window, LPARAM lParam)
{
    int width = LOWORD(lParam);
    int height = HIWORD(lParam);
    HWND child = GetWindow(window, GW_CHILD);
    MoveWindow(child, 1, 1, width - 2, height - 2, 0);
    InvalidateRect(child, 0, 0);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_PAINT:
		{
			paint(window);
			break;
		}
        case WM_SIZE:
        {
            handleSizeChanged(window, lParam);
            break;
        }
        case WM_RESIZE:
        {
            handleResize(state, window, lParam);
            break;
        }
        case WM_SETSTARTFRAME:
        {
            handleStartFrameChange(state, wParam);
            break;
        }
        case WM_NCHITTEST:
        {
            return HTTRANSPARENT;
        }
	}
	return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
