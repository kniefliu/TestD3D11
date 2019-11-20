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
	RECT rc = { 0,0,800,600 };
	DWORD dwStyle = WS_OVERLAPPED;
#if USE_POPUP
	window->Create(NULL, L"GDIWindow", WS_POPUP, 0, rc);
#else
	window->Create(NULL, L"GDIWindow", dwStyle, 0, rc);
#endif
	window->CenterWindow();
	window->ShowWindow();
	CWindowWnd::MessageLoop();

	return 0;
}
