#include "include.hpp"
#include "main.hpp"

int WINAPI wWinMain(_In_ HINSTANCE windowInstance, _In_opt_ HINSTANCE previousInstance, _In_ LPWSTR commandline, _In_ int showState)
{
    NOT_USING(windowInstance);
	NOT_USING(previousInstance);
	NOT_USING(commandline);
	NOT_USING(showState);

    GlobalState globalState;
    setGlobalState(&globalState);
    mainWindow::create(&globalState);

    startMessageLoop();
    return 0;
}

