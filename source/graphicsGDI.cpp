#include "include.hpp"
#include "graphicsGDI.hpp"

void rectangleFrame(HDC deviceContext, RECT* invalidRectangle, uint color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FrameRect(deviceContext, invalidRectangle, brush);
	DeleteObject(brush);
}
void rectangleFill(HDC deviceContext, RECT* invalidRectangle, uint color)
{
	HBRUSH brush = CreateSolidBrush(color);
	FillRect(deviceContext, invalidRectangle, brush);
	DeleteObject(brush);
}
void drawLine(HDC deviceContext, POINT* start, POINT* end)
{
    int x0 = start->x;
    int y0 = start->y;
    int x1 = end->x;
    int y1 = end->y;

    MoveToEx(deviceContext, x0, y0, 0);
    LineTo(deviceContext, x1, y1);
}
void drawGrid(HDC deviceContext, RECT* boundingBox, float spacing, int offsetX)
{
	int leftMarking = boundingBox->left;
	int rightMarking = boundingBox->right;

    int top = boundingBox->top;
    int bottom = boundingBox->bottom;

	for (int i = leftMarking; i != rightMarking + 1; ++i)
	{
		int x = (int)((float)i * spacing) - offsetX;
		MoveToEx(deviceContext, x, top, 0);
		LineTo(deviceContext, x, bottom);
	}
}
void drawMarking(HDC deviceContext, RECT* boundingBox, float spacing, int offsetX, int multiplier)
{
	HFONT font = CreateFont(15, 0, 0, 0, 0, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
	SelectObject(deviceContext, font);

	SetTextAlign(deviceContext, TA_LEFT);
	SetBkMode(deviceContext, TRANSPARENT);
	SetTextColor(deviceContext, COLOR_WHITE);

	int leftMarking = boundingBox->left;
	int rightMarking = boundingBox->right;
    int top = boundingBox->top;

	WCHAR marking[4] = {};
	for (int i = leftMarking; i != rightMarking + 1; ++i)
	{
		int x = (int)((float)i * spacing) - offsetX;
		wsprintf(marking, L"%d", i * multiplier);
		int stringLength = (int)wcslen(marking);
		TextOut(deviceContext, x + 4, top, marking, stringLength);
	}
	DeleteObject(font);
}
