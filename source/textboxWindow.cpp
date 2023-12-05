#include "include.hpp"
#include "textboxWindow.hpp"

START_SCOPE(textboxWindow)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(String* text, RECT* boundingBox, HWND parent, HWND* window)
{
	State* state = {};
	allocateMemory(sizeof(State), (void**)&state);
    state->text = *text;

	createWindowClass(L"textBoxWindowClass", windowCallback);
	createChildWindow(L"textBoxWindowClass", parent, state, window);

    int x = boundingBox->left;
    int y = boundingBox->top;
    int width = boundingBox->right;
    int height = boundingBox->bottom;
    placeWindow(*window, x, y, width, height);
}
void paintWindow(State* state, HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);
    int width, height;
    getWindowDimension(window, &width, &height);
    *invalidRectangle = {0, 0, width, height};
    drawText(deviceContext, &state->text, invalidRectangle, COLOR_BLACK);

	EndPaint(window, &paintStruct);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch(message)
	{
        case WM_PAINT:
        {
            paintWindow(state, window);
            break;
        }
	}
	return defaultWindowCallback(window, message, wParam, lParam);
}

END_SCOPE
