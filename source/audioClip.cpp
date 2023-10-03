#include "header.h"
#include "audioClip.h"

START_SCOPE(audioClip)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window, AudioClip* audioClip)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->audioClip = audioClip;

	createWindowClass(L"audioClipWindowClass", windowCallback);
	createChildWindow(L"audioClipWindowClass", parent, window, state);

	HWND waveform;
	waveform::create(*window, &waveform, &audioClip->waveFile);
	state->waveform = waveform;
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
void resizeChild(State* state, LPARAM lParam)
{
	HWND waveform = state->waveform;
	SendMessage(waveform, WM_RESIZE, 0, lParam);
}
LRESULT handleClientPreservation(State* state, WPARAM wParam, LPARAM lParam)
{
	if (!wParam)
	{
		return 0;
	}
	NCCALCSIZE_PARAMS* parameter = (NCCALCSIZE_PARAMS*)lParam;
	RECT* newRectangle = parameter->rgrc;
	RECT* oldRectangle = newRectangle + 1;
	LRESULT result = {};

	int width = newRectangle->right - newRectangle->left;
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 frameCount = (uint64)(width * framesPerPixel);
	state->audioClip->frameCount = frameCount;
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
void handleMove(State* state, LPARAM lParam)
{
	int x = LOWORD(lParam);
	sint64 framesPerPixel = globalState.framesPerPixel;
	uint64 startFrame = (uint64)(x * framesPerPixel);

	AudioClip* audioClip = state->audioClip;
	uint64 frameCount = audioClip->frameCount;
	uint64 endFrame = startFrame + frameCount;

	uint loadFrameCount = globalState.audioEndpointFrameCount; 
	state->audioClip->startFrame = startFrame / loadFrameCount;
	state->audioClip->endFrame = endFrame / loadFrameCount;
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
			paintWindow(window);
			break;
		}
		case WM_MOVING:
		{
			handleMoving(window, lParam);
			break;
		}
		case WM_MOVE:
		{
			handleMove(state, lParam);
			break;
		}
		case WM_NCCALCSIZE:
		{
			return handleClientPreservation(state, wParam, lParam);
		}
		case WM_NCHITTEST:
		{
			return handleHitTesting(window, lParam);
		}
		case WM_SIZE:
		{
			resizeChild(state, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
