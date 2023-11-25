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

	HFONT font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(deviceContext, font);

	SetTextAlign(deviceContext, TA_LEFT);
	SetBkMode(deviceContext, TRANSPARENT);
	SetTextColor(deviceContext, COLOR_BLACK);

	String* text = &state->text;
	TextOut(deviceContext, 8, 2, text->string, (int)text->characterCount);

	DeleteObject(font);
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
