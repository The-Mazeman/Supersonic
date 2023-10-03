#include "header.h"
#include "trackControl.h"

START_SCOPE(trackControl)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void createLayout(State* state, HWND window, int width, int height)
{
	topbar::create(window, &state->topbar);
	WindowPosition fader = {0, height / 2, width, height / 2};
	fader::create(window, &state->volumeFader, &fader, FADER_0);
}
void setAttribute(HWND window)
{
	SetWindowTheme(window, L"", L"");
}
void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);
	state->audioTrack = 0;
	int width = globalState.trackControlWidth;
	int height = globalState.trackControlHeight;

	createWindowClass(L"trackControlWindowClass", windowCallback);
	createOwnedWindow(L"trackControlWindowClass", parent, window, state);
	createLayout(state, *window, width, height);
	setAttribute(*window);
	placeWindow(*window, 0, 0, width, height);
}
void setControl(State* state, WPARAM wParam, LPARAM lParam)
{
	HWND audioTrack = (HWND)wParam;
	TrackControl* trackControl = (TrackControl*)lParam;
	state->audioTrack = audioTrack;
	state->trackControl = *trackControl;
	HWND volumeFader = state->volumeFader;
	SendMessage(volumeFader, WM_SETCONTROL, (WPARAM)&trackControl->gain, 0);
}
void paintWindow(HWND window)
{
	PAINTSTRUCT paintStruct;
	HDC deviceContext = BeginPaint(window, &paintStruct);

	RECT* invalidRectangle = &paintStruct.rcPaint;
	rectangleFill(deviceContext, invalidRectangle, COLOR_BLACK);

	EndPaint(window, &paintStruct);
}
LRESULT handleHitTesting(HWND window, LPARAM lParam)
{
	int cursorY = GET_Y_LPARAM(lParam);

	int a, b, c, d;
	getWindowRectangle(window, &a, &b, &c, &d);

	if (cursorY < b + 16)
	{
		return HTCAPTION;
	}

	return HTCLIENT;
}
void handleSizeChanged(State* state, LPARAM lParam)
{
	int width = LOWORD(lParam);
	int height = HIWORD(lParam);

	POINT dimension = {width, height};
	HWND* childArray = (HWND*)state;
	for(uint i = 0; i != 2; ++i)
	{
		SendMessage(childArray[i], WM_RESIZE, (WPARAM)&dimension, 0);
	}
}
void setSizeLimit(LPARAM lParam)
{
	MINMAXINFO* size = (MINMAXINFO*)lParam;
	size->ptMinTrackSize.x = 0;
	size->ptMinTrackSize.y = 0;
}
void changeGain(TrackControl* trackControl, int faderValue)
{
	float db = (float)faderValue / 10.0f;

	float gain = (float)pow(10.0f, db / 20.0f);
	trackControl->gain = _mm256_broadcast_ss(&gain);
}
void faderMove(State* state, WPARAM wParam, LPARAM lParam)
{
	uint faderId = (uint)wParam;
	int faderValue = (int)lParam;
	TrackControl* trackControl = &state->trackControl;
	switch(faderId)
	{
		case FADER_0:
		{
			changeGain(trackControl, faderValue);
			break;
		}
		case FADER_1:
		{
		}
	}
	HWND audioTrack = state->audioTrack;
	if(audioTrack)
	{
		SendMessage(audioTrack, WM_SETCONTROL, (WPARAM)trackControl, 0);
	}
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
		case WM_SETCONTROL:
		{
			setControl(state, wParam, lParam);
			break;
		}
		case WM_FADERMOVE:
		{
			faderMove(state, wParam, lParam);
			break;
		}
		case WM_GETMINMAXINFO:
		{
			setSizeLimit(lParam);
			break;
		}
		case WM_SIZE:
		{
			handleSizeChanged(state, lParam);
			return 0;
		}
		case WM_NCCALCSIZE:
		{
			return 0;
		}
		case WM_NCACTIVATE:
		{
			lParam = -1;
		}
		case WM_NCPAINT:
		case WM_SETICON:
		case WM_SETTEXT:
		case WM_NCUAHDRAWCAPTION:
		case WM_NCUAHDRAWFRAME:
		{
			return 0;
		}
		case WM_NCHITTEST:
		{
			return handleHitTesting(window, lParam);
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}

END_SCOPE
