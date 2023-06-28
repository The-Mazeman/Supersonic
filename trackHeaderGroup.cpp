#include "header.h"
#include "platform.h"
#include "trackHeaderGroup.h"
#include "trackHeader.h"
#include "textbox.h"
#include "globalState.h"

START_SCOPE(trackHeaderGroup)

void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	SetProp(window, L"state", state);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	int top = invalidRectangle->top;
	int bottom = invalidRectangle->bottom;
	int width = globalState.sidebarWidth - 1;

	SelectObject(deviceContext, GetStockObject(DC_PEN));
	SetDCPenColor(deviceContext, COLOR_WHITE);

	MoveToEx(deviceContext, width, top, 0);
	LineTo(deviceContext, width, bottom);

	EndPaint(window, &paintStruct);
}
void createTrack(HWND window, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	HWND* trackHeaderArray = state->trackHeaderArray;
	int trackNumber = (int)lParam;

	AudioClip* audioClip = (AudioClip*)wParam;
	int width = globalState.sidebarWidth;

	HWND trackHeader = {};
	trackHeader::create(window, &audioClip->waveFile.name, trackNumber, width, &trackHeader);
	trackHeaderArray[trackNumber] = trackHeader;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createWindowClass(L"trackHeaderWindowClass", trackHeader::windowCallback);
			createWindowClass(L"textboxWindowClass", textbox::windowCallback);
			createState(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_RESIZE:
		{
			resizeWindow(window, lParam);
			break;
		}
		case WM_CREATETRACK:
		{
			createTrack(window, wParam, lParam);
			break;
		}
		case WM_VERTICALMOUSEWHEEL:
		{
			handleVerticalScroll(window, lParam);
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
