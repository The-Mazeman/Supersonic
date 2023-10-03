#include "header.h"
#include "main.h"

void initializeState()
{
	globalState.framesPerPixel = 512;
	globalState.sampleRate = 48000;
	globalState.trackCount = 0;
	globalState.trackHeight = 32;
	globalState.rulerHeight = 16;
	globalState.sidebarWidth = 128;
	globalState.topbarHeight = 16;
	globalState.trackControlWidth = 128;
	globalState.trackControlHeight = 512;
	globalState.offsetX = 0;
	globalState.offsetY = 0;
}
int WINAPI wWinMain(_In_ HINSTANCE windowInstance, _In_opt_ HINSTANCE previousInstance, _In_ LPWSTR arguments, _In_ int showState)
{
	notUsing(previousInstance);
	notUsing(arguments);
	notUsing(showState);
	notUsing(windowInstance);
	
    initializeState();
    mainWindow::create();
    startMessageLoop();

	return 0;
}
