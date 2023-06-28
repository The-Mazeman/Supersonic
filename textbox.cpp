#include "header.h"
#include "platform.h"

START_SCOPE(textbox)

void create(HWND window, String* text, int width)
{
	HWND textBox;
	createLayer(L"textBoxWindowClass", window, &textBox);
	SetProp(textBox, L"text", text);
	MoveWindow(textBox, 0, 0, 140, 20, 1);
}
void paintWindow(HWND window)
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

	String* text = (String*)GetProp(window, L"text");
	TextOut(deviceContext, 2, 2, text->string, (int)text->characterCount);


	DeleteObject(font);

	EndPaint(window, &paintStruct);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE