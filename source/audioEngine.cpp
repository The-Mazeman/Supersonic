#include "include.hpp"
#include "audioEngine.hpp"

START_SCOPE(audioEngine)

LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

void create(HWND parent, HWND* window)
{
	State* state = {};
	allocateMemory(sizeof(State), (void**)&state);

	createWindowClass(L"audioEngineWindowClass", windowCallback);
	createChildWindow(L"audioEngineWindowClass", parent, state, window);
}
void createChild(State* state, HWND window)
{
}
void startPlayback(State* state, WPARAM wParam)
{
}
void stopPlayback(State* state)
{
    HWND wasapi = state->wasapi;
    SendMessage(wasapi, WM_PAUSE, 0, 0);
}
LRESULT windowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    State* state = (State*)GetProp(window, L"state");
    switch(message)
    {
        case WM_CREATECHILD:
        {
            createChild(state, window);
            break;
        }
        case WM_PLAY:
        {
            startPlayback(state, wParam);
            break;
        }
        case WM_PAUSE:
        {
            stopPlayback(state);
            break;
        }
    }
    return defaultWindowCallback(window, message, wParam, lParam);
}
END_SCOPE
