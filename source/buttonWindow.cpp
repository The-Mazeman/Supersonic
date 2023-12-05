#include "include.hpp"
#include "buttonWindow.hpp"

START_SCOPE(buttonWindow)

LRESULT CALLBACK windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(String* buttonString, uint buttonId, RECT* boundingBox, HWND parent, HWND* window)
{
    State* state = {};
    allocateMemory(sizeof(State), (void**)&state);
    state->string = *buttonString;
    state->pressed = 0;
    state->buttonId = buttonId;

    createWindowClass(L"buttonWindowClass", windowCallback);
    createChildWindow(L"buttonWindowClass", parent, state, window);

    placeWindow(*window, boundingBox); 
}
void paint(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

    uint textColor = {};
    uint backgroundColor = {};
    if(state->pressed)
    {
        textColor = COLOR_BLACK;
        backgroundColor = COLOR_WHITE;
    }
    else
    {
        textColor = COLOR_WHITE;
        backgroundColor = COLOR_BLACK;
    }

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, backgroundColor);

    int width, height;
    getWindowDimension(window, &width, &height);
    *invalidRectangle = {0, 0, width, height};
	rectangleFrame(deviceContext, invalidRectangle, COLOR_WHITE);

    drawText(deviceContext, &state->string, invalidRectangle, textColor);

	EndPaint(window, &paintStruct);
}
void handleMouseLeftClick(State* state, HWND window)
{
    state->pressed = !state->pressed;
    InvalidateRect(window, 0, 0);
    HWND parent = GetAncestor(window, GA_PARENT);
    SendMessage(parent, WM_BUTTONPRESSED, state->pressed, state->buttonId);
}
void getButtonState(State* state, WPARAM wParam)
{
    uint* buttonState = (uint*)wParam;
    *buttonState = state->pressed;
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
        case WM_LBUTTONDOWN:
        {
            handleMouseLeftClick(state, window);
            break;
        }
        case WM_GETBUTTONSTATE:
        {
            getButtonState(state, wParam);
            break;
        }
        case WM_NCHITTEST:
        {
            return HTCLIENT;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
