#include "header.h"
#include "platform.h"
#include "globalState.h"
#include "mainWindow.h"
#include "topbar.h"

void setupWindow()
{
	createWindowClass(L"mainWindowClass", main::windowCallback);
    HWND window; 
	createWindow(L"mainWindowClass", &window);

    SetWindowTheme(window, L"", L"");
	DragAcceptFiles(window, 1);
	MoveWindow(window, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
	ShowWindow(window, SW_SHOW);
}
int WINAPI wWinMain(_In_ HINSTANCE windowInstance, _In_opt_ HINSTANCE previousInstance, _In_ LPWSTR arguments, _In_ int showState)
{
    notUsing(previousInstance);
    notUsing(arguments);
    notUsing(showState);
    notUsing(windowInstance);

	initializeGlobalState();
    setupWindow();
    startMessageLoop();

	return 0;   
}
