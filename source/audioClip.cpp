#include "header.h"
#include "audioClip.h"

START_SCOPE(audioClip)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* audioClip)
{
	createWindowClass(L"audioClipWindowClass", windowCallback);
	createChildWindow(L"audioClipWindowClass", window, audioClip);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFrame(deviceContext, invalidRectangle, COLOR_WHITE);

	EndPaint(window, &paintStruct);
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorX = GET_X_LPARAM(lParam);
	//int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if (cursorX < a + 4)
	{
		return HTLEFT;
	}
	if (cursorX > c - 4)
	{
		return HTRIGHT;
	}
	return HTCAPTION;
}
void handleMoving(HWND window, LPARAM lParam)
{
	RECT* windowRectangle = (RECT*)lParam;
	int x, y;
	getWindowPosition(window, &x, &y);
	windowRectangle->top = y;
	windowRectangle->bottom = y + globalState.trackHeight;
}
void resizeChild(HWND window, LPARAM lParam)
{
	HWND child = GetWindow(window, GW_CHILD);
	SendMessage(child, WM_RESIZE, 0, lParam);
}
LRESULT handleClientPreservation(WPARAM wParam, LPARAM lParam)
{
	if (!wParam)
	{
		return 0;
	}
	NCCALCSIZE_PARAMS* parameter = (NCCALCSIZE_PARAMS*)lParam;
	RECT* newRectangle = parameter->rgrc;
	RECT* oldRectangle = newRectangle + 1;
	LRESULT result = {};

	//int startOffsetDelta = newRectangle->right - oldRectangle->right;
	//int endOffsetDelta = oldRectangle->left -  newRectangle->left;


	if (newRectangle->right == oldRectangle->right)
	{
		result = WVR_ALIGNRIGHT;
	}
	return result;
}
#if 0
void handleSizing(WPARAM wParam, LPARAM lParam)
{
	if(wParam == WMSZ_LEFT)
	{
	}
}
#endif
void handleFileDrop(State* state, HWND window, WPARAM wParam)
{
	AudioClip* audioClip = (AudioClip*)wParam;
	state->audioClip = audioClip;
	HWND waveform;
	waveform::create(window, &waveform);
	SendMessage(waveform, WM_FILEDROP, (WPARAM)&audioClip->waveFile, 0);
}
void initialize(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	SetProp(window, L"state", state);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	switch (message)
	{
		case WM_CREATE:
		{
			initialize(window);
			break;
		}
		case WM_PAINT:
		{
			paintWindow(window);
			break;
		}
		case WM_FILEDROP:
		{
			handleFileDrop(state, window, wParam);
			break;
		}
		case WM_MOVING:
		{
			handleMoving(window, lParam);
			break;
		}
		case WM_NCCALCSIZE:
		{
			return handleClientPreservation(wParam, lParam);
		}
		case WM_NCHITTEST:
		{
			return handleHitTesting(window, lParam);
		}
		case WM_SIZING:
		{
			//handleSizing(wParam, lParam);
		}
		case WM_SIZE:
		{
			resizeChild(window, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
