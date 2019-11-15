#include <Windows.h>

#include "D3D11BaseWindow.h"

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR    lpCmdLine,
	int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	D3D11BaseWindow* window = new D3D11BaseWindow();
	window->Create(NULL, L"D3D11Window", UI_WNDSTYLE_FRAME, 0);
	CWindowWnd::MessageLoop();

	return 0;
}
