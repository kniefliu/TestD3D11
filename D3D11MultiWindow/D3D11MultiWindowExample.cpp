#include <Windows.h>

#include "GDIBaseWindow.h"

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	GDIBaseWindow* window = new GDIBaseWindow();
	window->Create(NULL, L"GDIWindow", UI_WNDSTYLE_FRAME, 0);
	CWindowWnd::MessageLoop();

	return 0;
}
