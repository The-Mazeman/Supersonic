#include "include.hpp"
#include "rulerWindow.hpp"

START_SCOPE(rulerWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(GlobalState* globalState, HWND parent, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->globalState = globalState;
	state->x = (int)globalState->sidebarWidth;
	state->y = (int)globalState->titleBarHeight;
    state->scalar = 1;

    createWindowClass(L"rulerWindowClass", windowCallback);
    createChildWindow(L"rulerWindowClass", parent, state, window);
}
void paint(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);
    int width, height;
    getWindowDimension(window, &width, &height);

	int left = invalidRectangle->left;
	int right = invalidRectangle->right;
    POINT start = {left, height - 1};
    POINT end = {right, height - 1};

    SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);
    drawLine(deviceContext, &start, &end);

    int framesPerPixel = (int)state->globalState->framesPerPixel;
    int sampleRate = (int)state->globalState->sampleRate;
    float spacing = (float)sampleRate / (float)framesPerPixel;
    int offsetX = (int)state->globalState->offsetX;
    left += offsetX;
    right += offsetX;

    int scalar = state->scalar;
    spacing *= (float)scalar;
    if(spacing < 64)
    {
        scalar *= 2;
        spacing *= 2;
        state->scalar = scalar;
    }
    if(spacing > 128 && scalar > 1)
    {
        scalar /= 2;
        spacing /= 2;
        state->scalar = scalar;
    }

	int leftMarking = (int)((float)left / spacing);
	int rightMarking = (int)((float)right / spacing);

    invalidRectangle->left = leftMarking;
    invalidRectangle->right = rightMarking;
    invalidRectangle->bottom = height / 2;

    drawGrid(deviceContext, invalidRectangle, spacing, offsetX);
    drawMarking(deviceContext, invalidRectangle, spacing, offsetX, scalar);

	EndPaint(window, &paintStruct);
}
void handleResize(State* state, HWND window, LPARAM lParam)
{
    int x = state->x;
	int y = state->y;
    int width = LOWORD(lParam) - x;
    int height = (int)state->globalState->rulerHeight;
    placeWindow(window, x, y, width, height);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_PAINT:
        {
            paint(state, window);
            break;
        }
        case WM_RESIZE:
        {
            handleResize(state, window, lParam);
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
