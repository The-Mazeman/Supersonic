#include "include.hpp"
#include "titleBarWindow.hpp"

START_SCOPE(titleBarWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(GlobalState* globalState, HWND parent, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->globalState = globalState;
	state->x = 0;
	state->y = 0;

    createWindowClass(L"titleBarWindowClass", windowCallback);
    createChildWindow(L"titleBarWindowClass", parent, state, window);
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
    int width = LOWORD(lParam);
    int height = (int)state->globalState->titleBarHeight;
    int x = state->x;
	int y = state->y;
    placeWindow(window, x, y, width, height);
}
LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_PAINT:
        {
            paint(window);
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
