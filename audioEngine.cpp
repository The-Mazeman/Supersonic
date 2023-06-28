#include "header.h"
#include "audioEngine.h"

START_SCOPE(audioEngine)

void createState(HWND window)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (char**)&state);
	SetProp(window, L"state", state);

	createWindowClass(L"audioTrackWindowClass", audioTrack::windowCallback);
}
#if 0

void handleFileDrop(HWND window, WPARAM wParam, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	int trackNumber = (int)lParam;
	DWORD trackThreadId = state->trackThreadArray[trackNumber];
	PostThreadMessage(trackThreadId, WM_FILEDROP, wParam, 0);
}
void createTrack(HWND window, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	AudioTrack* audioTrack;
	allocateSmallMemory(sizeof(AudioTrack), (char**)&audioTrack);
	audioTrack->clipCount = 0;
	audioTrack->clipList = 0;

	int trackNumber = (int)lParam;

	HANDLE loadEvent;
	createEvent(0, &loadEvent);
	audioTrack->loadEvent = loadEvent;
	audioTrack->busSemaphore = state->busSemaphore;
	state->trackLoadEvent[trackNumber] = loadEvent;

	DWORD trackThreadId;
	createThread(trackThread, audioTrack, &trackThreadId);
	state->trackThreadArray[trackNumber] = trackThreadId;

	WaitForSingleObject(loadEvent, INFINITE);
}
#endif

void setCursor(HWND window, WPARAM wParam)
{
	State* state = (State*)GetProp(window, L"state");
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_SETCURSOR, wParam, 0);
}
void createWasapiController(HWND window)
{
	HWND wasapi;
	createWindowClass(L"wasapiWindowClass", wasapi::windowCallback);
	createChildWindow(L"wasapiWindowClass", window, &wasapi);

	State* state = (State*)GetProp(window, L"state");
	state->wasapi = wasapi;
}
void startPlayback(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_GETOUTPUT, (WPARAM)&state->outputBuffer, (LPARAM)&state->loadEvent);
	SendMessage(wasapi, WM_PLAY, 0, 0);
}
void stopPlayback(HWND window)
{
	State* state = (State*)GetProp(window, L"state");
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_PAUSE, 0, 0);
}
void createTrack(HWND window, LPARAM lParam)
{
	State* state = (State*)GetProp(window, L"state");
	int trackNumber = (int)lParam;

	HWND audioTrack;
	createChildWindow(L"audioTrackWindowClass", window, &audioTrack);
	state->audioTrackArray[trackNumber] = audioTrack;
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			createState(window);
			createWasapiController(window);
			break;
		}
		case WM_PLAY:
		{
			startPlayback(window);
			break;
		}
		case WM_PAUSE:
		{
			stopPlayback(window);
			break;
		}
		case WM_SETCURSOR:
		{
			setCursor(window, wParam);
			break;
		}
		case WM_TIMER:
		{
			//handleTimer(window);
			break;
		}
		case WM_CREATETRACK:
		{
			createTrack(window, lParam);
			break;
		}
		case WM_FILEDROP:
		{
			//handleFileDrop(window, wParam, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
END_SCOPE