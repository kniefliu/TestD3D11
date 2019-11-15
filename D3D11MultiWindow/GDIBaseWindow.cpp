#include "GDIBaseWindow.h"

#include <windowsx.h>

GDIBaseWindow::GDIBaseWindow()
	: m_pChildWindow(nullptr)
{

}
GDIBaseWindow::~GDIBaseWindow()
{

}

LPCTSTR GDIBaseWindow::GetWindowClassName() const
{
	return L"GDIBaseWindow";
}
UINT GDIBaseWindow::GetClassStyle() const
{
	return CS_VREDRAW | CS_HREDRAW;
}
LRESULT GDIBaseWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
	{
		m_pChildWindow = new D3D11BaseWindow();
		m_pChildWindow->Create(m_hWnd, L"D3D11BaseWindow", UI_WNDSTYLE_CHILD, 0);
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT client_rc;
		::GetClientRect(m_hWnd, &client_rc);
		HBRUSH hbrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
		::FillRect(hdc, &client_rc, hbrush);
		return 0;
	}
	case WM_SIZE:
	{
		if (m_pChildWindow) {
			int cx = LOWORD(lParam) - 40;
			int cy = HIWORD(lParam) - 40;
			m_pChildWindow->Resize(cx, cy);
		}
		return 0;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = ::BeginPaint(m_hWnd, &ps);
		DoPaint(hdc);
		::EndPaint(m_hWnd, &ps);
		return 0;
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}
void GDIBaseWindow::OnFinalMessage(HWND hWnd)
{
	delete this;
}
void GDIBaseWindow::DoPaint(HDC hdc)
{
	RECT client_rc;
	::GetClientRect(m_hWnd, &client_rc);

	::InflateRect(&client_rc, -20, -20);
	HBRUSH hbrush = (HBRUSH)::GetStockObject(GRAY_BRUSH);
	::FillRect(hdc, &client_rc, hbrush);
}
