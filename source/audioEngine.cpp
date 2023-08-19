#include "header.h"
#include "audioEngine.h"

START_SCOPE(audioEngine)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND window, HWND* audioEngine)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);

    createWindowClass(L"audioEngineWindowClass", windowCallback);
    createChildWindow(L"audioEngineWindowClass", window, audioEngine, state);

	wasapi::create(window, &state->wasapi);
	trackControl::create(window, &state->trackControl);
}
void setCursor(State* state, WPARAM wParam)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_SETCURSOR, wParam, 0);
}
void startPlayback(State* state)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_STARTLOADER, 0, 0);
	SendMessage(wasapi, WM_PLAY, 0, 0);
}
void stopPlayback(State* state)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_PAUSE, 0, 0);
}
void createTrack(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
    uint trackNumber = (uint)wParam;
    HWND audioTrack;
    audioTrack::create(window, &audioTrack);
    state->audioTrackArray[trackNumber] = audioTrack;
	*(HWND*)lParam = audioTrack;
}
void handleFileDrop(State* state, WPARAM wParam, LPARAM lParam)
{
	int trackNumber = (int)lParam;
	HWND audioTrack = state->audioTrackArray[trackNumber];
	SendMessage(audioTrack, WM_FILEDROP, wParam, 0);
}
void assignBus(State* state, WPARAM wParam, LPARAM lParam)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_ASSIGNBUS, wParam, lParam);
}
void setControl(State* state, WPARAM wParam)
{
	HWND trackControl = state->trackControl;
	SendMessage(trackControl, WM_SETCONTROL, wParam, 0);
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
		case WM_PLAY:
		{
			startPlayback(state);
			break;
		}
		case WM_PAUSE:
		{
			stopPlayback(state);
			break;
		}
		case WM_SETCURSOR:
		{
			setCursor(state, wParam);
			break;
		}
		case WM_CREATETRACK:
		{
			createTrack(state, window, wParam, lParam);
			break;
		}
		case WM_FILEDROP:
		{
			handleFileDrop(state, wParam, lParam);
			break;
		}
		case WM_ASSIGNBUS:
		{
			assignBus(state, wParam, lParam);
			break;
		}
		case WM_SETCONTROL:
		{
			setControl(state, wParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
END_SCOPE
