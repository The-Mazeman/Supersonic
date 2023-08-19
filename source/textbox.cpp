#include "header.h"
#include "textbox.h"

START_SCOPE(textbox)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* textBox)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->name.string = (WCHAR*)L"Audio Track";
	state->name.characterCount = 12;

	createWindowClass(L"textBoxWindowClass", windowCallback);
	createChildWindow(L"textBoxWindowClass", window, textBox, state);
	MoveWindow(*textBox, 0, 0, 140, 20, 1);
}
void paintWindow(State* state, HWND window)
{
	notUsing(state);
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_WHITE);

	HFONT font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(deviceContext, font);

	SetTextAlign(deviceContext, TA_LEFT);
	SetBkMode(deviceContext, TRANSPARENT);
	SetTextColor(deviceContext, COLOR_BLACK);

	String* name = &state->name;
	TextOut(deviceContext, 2, 2, name->string, (int)name->characterCount);

	DeleteObject(font);

	EndPaint(window, &paintStruct);
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
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
