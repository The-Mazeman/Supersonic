#include "header.h"
#include "audioEngine.h"

START_SCOPE(audioEngine)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* audioEngine)
{
	State* state = {};
	allocateSmallMemory(sizeof(State), (void**)&state);

	createArray(&state->busArrayHandle, sizeof(HWND));
	createArray(&state->audioTrackArrayHandle, sizeof(HWND));
	createArray(&state->trackProcessorStartEventArrayHandle, sizeof(HWND));

    createWindowClass(L"audioEngineWindowClass", windowCallback);
    createChildWindow(L"audioEngineWindowClass", parent, audioEngine, state);

	wasapi::create(*audioEngine, &state->wasapi);
	HWND masterBus = {};
    masterBus::create(*audioEngine, &masterBus, state->wasapi);
	arrayAppend(state->busArrayHandle, &masterBus);
	trackControl::create(*audioEngine, &state->trackControl);
}
void setCursor(State* state, WPARAM wParam)
{
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_SETCURSOR, wParam, 0);
}
void sendOutput(HWND* audioTrackArray, uint trackCount)
{
	for(uint i = 0; i != trackCount; ++i)
	{
		SendMessage(audioTrackArray[i], WM_SENDOUTPUTLOADER, 0, 0);
	}
}
void createBusBuffer(HWND* busArray, uint busCount)
{
	for (uint i = 0; i != busCount; ++i)
	{
		SendMessage(busArray[i], WM_CREATEBUFFER, 0, 0);
	}
}
void createInputLoader(HWND* busArray, uint busCount)
{
	for (uint i = 0; i != busCount; ++i)
	{
		SendMessage(busArray[i], WM_STARTINPUTLOADER, 0, 0);
	}
}
void createTrackBuffer(HWND* audioTrackArray, HANDLE* trackProcessorStartEventArray, uint trackCount)
{
	for(uint i = 0; i != trackCount; ++i)
	{
		SendMessage(audioTrackArray[i], WM_CREATEBUFFER, (WPARAM)trackProcessorStartEventArray[i], 0);
	}
}
void startPlayback(State* state, WPARAM wParam)
{
	notUsing(wParam);
    uint trackCount = {};
	void* audioTrackArrayHandle = state->audioTrackArrayHandle;
    HWND* audioTrackArray = {};
	getArray(audioTrackArrayHandle, (void**)&audioTrackArray, &trackCount);
	sendOutput(audioTrackArray, trackCount);

	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_SENDINPUTLOADER, 0, 0);

    HANDLE* trackProcessorStartEventArray = {};
	void* trackProcessorStartEventArrayHandle = state->trackProcessorStartEventArrayHandle;
	getArrayStart(trackProcessorStartEventArrayHandle, (void**)&trackProcessorStartEventArray);
	createTrackBuffer(audioTrackArray, trackProcessorStartEventArray, trackCount);

	HWND* busArray = {};
	void* busArrayHandle = state->busArrayHandle;
	uint busCount = {};
	getArray(busArrayHandle, (void**)&busArray, &busCount);
	createBusBuffer(busArray, busCount);

	createInputLoader(busArray, busCount);
	setEventArray(trackProcessorStartEventArray, trackCount);

	SendMessage(wasapi, WM_STARTOUTPUTLOADER, (WPARAM)trackProcessorStartEventArray, (LPARAM)trackCount);
	SendMessage(wasapi, WM_STARTINPUTLOADER, 0, 0);
	SendMessage(wasapi, WM_PLAY, 0, 0);
}
void stopPlayback(State* state, WPARAM wParam)
{
	notUsing(wParam);
	HWND wasapi = state->wasapi;
	SendMessage(wasapi, WM_PAUSE, 0, 0);

	HWND* audioTrackArray = {};
	void* audioTrackArrayHandle = state->audioTrackArrayHandle;
	uint trackCount = {};
	getArray(audioTrackArrayHandle, (void**)&audioTrackArray, &trackCount);
    for(uint i = 0; i != trackCount; ++i)
    {
        SendMessage(audioTrackArray[i], WM_PAUSE, 0, 0);
    }
	void* busArrayHandle = state->busArrayHandle;
	HWND* busArray = {};
	uint busCount = {};
	getArray(busArrayHandle, (void**)&busArray, &busCount);
	for(uint i = 0; i != busCount; ++i)
	{
		SendMessage(busArray[i], WM_PAUSE, 0, 0);
	}
}
void createTrack(State* state, HWND window, WPARAM wParam, LPARAM lParam)
{
	notUsing(wParam);
    HWND* audioTrack = (HWND*)lParam;
    audioTrack::create(window, audioTrack);
	void* audioTrackArrayHandle = state->audioTrackArrayHandle;
	arrayAppend(audioTrackArrayHandle, audioTrack);

	HANDLE trackProcessorStartEvent = {};
	createEvent(0, &trackProcessorStartEvent);
	void* trackProcessorStartEventArrayHandle = state->trackProcessorStartEventArrayHandle;
	arrayAppend(trackProcessorStartEventArrayHandle, &trackProcessorStartEvent);
}
void handleFileDrop(State* state, WPARAM wParam, LPARAM lParam)
{
	int trackNumber = (int)lParam;
	HWND* audioTrackArray = {};
	void* audioTrackArrayHandle = state->audioTrackArrayHandle;
	getArrayStart(audioTrackArrayHandle, (void**)&audioTrackArray);
	HWND audioTrack = audioTrackArray[trackNumber];
	SendMessage(audioTrack, WM_FILEDROP, wParam, 0);
}
void sendControl(State* state, WPARAM wParam, LPARAM lParam)
{
	HWND trackControl = state->trackControl;
	SendMessage(trackControl, WM_SETCONTROL, wParam, lParam);
}
void setInputLoader(State* state, WPARAM wParam, LPARAM lParam)
{
	uint busNumber = (uint)lParam;
	void* busArrayHandle = state->busArrayHandle;
	HWND* busArray = {};
	getArrayStart(busArrayHandle, (void**)&busArray);
	HWND bus = busArray[busNumber];
	SendMessage(bus, WM_SETINPUTLOADER, wParam, 0);
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
			startPlayback(state, wParam);
			break;
		}
		case WM_PAUSE:
		{
			stopPlayback(state, wParam);
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
		case WM_SENDCONTROL:
		{
			sendControl(state, wParam, lParam);
			break;
		}
		case WM_SETINPUTLOADER:
		{
			setInputLoader(state, wParam, lParam);
			break;
		}
	}
	return DefWindowProc(window, message, wParam, lParam);
}
END_SCOPE
