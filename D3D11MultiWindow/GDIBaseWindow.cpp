#include "GDIBaseWindow.h"

#include <windowsx.h>
#include <dwmapi.h>

const int kDefaultBorderSize = 4;
const int kDefaultCaptionHeight = 20;

#define WM_RESIZE  WM_USER + 1

GDIBaseWindow::GDIBaseWindow()
	: m_pChildWindow(nullptr)
	, m_hbrushFill(nullptr)
{
	m_rcSizeBox.left = kDefaultBorderSize;
	m_rcSizeBox.right = kDefaultBorderSize;
	m_rcSizeBox.top = kDefaultBorderSize;
	m_rcSizeBox.bottom = kDefaultBorderSize;

	m_rcCaption.left = 0;
	m_rcCaption.top = 0;
	m_rcCaption.bottom = kDefaultCaptionHeight;
	m_rcCaption.right = 0;
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
static HRESULT DisableNCRendering(HWND hWnd)
{
	HRESULT hr = S_OK;

	DWMNCRENDERINGPOLICY ncrp = DWMNCRP_DISABLED;

	// Disable non-client area rendering on the window.
	hr = ::DwmSetWindowAttribute(hWnd,
		DWMWA_NCRENDERING_POLICY,
		&ncrp,
		sizeof(ncrp));

	if (SUCCEEDED(hr))
	{
		// ...
	}

	return hr;
}
LRESULT GDIBaseWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_NCACTIVATE:
	{
		if (!::IsIconic(*this)) {
			return (wParam == 0) ? TRUE : FALSE;
		}
	}
	case WM_NCPAINT:
	{
		return 1;
	}
	case WM_PARENTNOTIFY:
	{
		break;
	}
	case WM_NCHITTEST:
	{
		POINT pt; pt.x = GET_X_LPARAM(lParam); pt.y = GET_Y_LPARAM(lParam);
		::ScreenToClient(*this, &pt);

		RECT rcClient;
		::GetClientRect(*this, &rcClient);

		if (!::IsZoomed(*this)) {
			if (pt.y < rcClient.top + m_rcSizeBox.top) {
				if (pt.x < rcClient.left + m_rcSizeBox.left) return HTTOPLEFT;
				if (pt.x > rcClient.right - m_rcSizeBox.right) return HTTOPRIGHT;
				return HTTOP;
			}
			else if (pt.y > rcClient.bottom - m_rcSizeBox.bottom) {
				if (pt.x < rcClient.left + m_rcSizeBox.left) return HTBOTTOMLEFT;
				if (pt.x > rcClient.right - m_rcSizeBox.right) return HTBOTTOMRIGHT;
				return HTBOTTOM;
			}
			if (pt.x < rcClient.left + m_rcSizeBox.left) return HTLEFT;
			if (pt.x > rcClient.right - m_rcSizeBox.right) return HTRIGHT;
		}

		if (pt.x >= rcClient.left + m_rcCaption.left && pt.x < rcClient.right - m_rcCaption.right \
			&& pt.y >= m_rcCaption.top && pt.y < m_rcCaption.bottom) {
			return HTCAPTION;
		}
		return HTCLIENT;
	}
	case WM_CREATE:
	{
		m_hbrushFill = (HBRUSH)::GetStockObject(LTGRAY_BRUSH);

		LONG styleValue = ::GetWindowLong(*this, GWL_STYLE);
		styleValue &= ~WS_CAPTION;
		styleValue &= ~WS_BORDER;
		::SetWindowLong(*this, GWL_STYLE, styleValue | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
		//DisableNCRendering(m_hWnd);

#if USE_GDI_CHILD_WINDOW
		if ((styleValue & WS_CHILD) == WS_CHILD) {
			m_hbrushFill = (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
		}
		else {
			m_pChildWindow = new GDIBaseWindow();
			m_pChildWindow->Create(m_hWnd, L"GDIChildWindow", UI_WNDSTYLE_CHILD, 0);
		}
#else
		if ((styleValue & WS_CHILD) == WS_CHILD) {
			m_hbrushFill = (HBRUSH)::GetStockObject(DKGRAY_BRUSH);
		}
		else {
			m_pChildWindow = new D3D11BaseWindow();
			m_pChildWindow->Create(m_hWnd, L"D3D11BaseWindow", UI_WNDSTYLE_CHILD, 0);
			//::SetTimer(m_hWnd, kAnimationTimer, 15, 0);
		}
#endif
		
		return 0;
	}
	case WM_RESIZE:
	{
		::SetWindowPos(m_hWnd, NULL, 0, 0, (int)wParam, (int)lParam, SWP_NOMOVE);
		return 0;
	}
	case WM_TIMER:
	{
		if (wParam == kAnimationTimer) {
#ifndef USE_GDI_CHILD_WINDOW
			RECT rc;
			::GetWindowRect(m_hWnd, &rc);
			static bool alert = true;
			if (alert) {
				rc.right += 2;
				rc.bottom += 2;
			}
			else {
				rc.right -= 2;
				rc.bottom -= 2;
			}
			::PostMessage(m_hWnd, WM_RESIZE, (rc.right - rc.left), (rc.bottom - rc.top));
			alert = !alert;
#endif
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		HDC hdc = (HDC)wParam;
		RECT client_rc;
		::GetClientRect(m_hWnd, &client_rc);
		HBRUSH hbrush = (HBRUSH)::GetStockObject(WHITE_BRUSH);
		::FillRect(hdc, &client_rc, hbrush);
		return 1;
	}
	case WM_SIZE:
	{
		if (m_pChildWindow) {
			int cx = LOWORD(lParam) - 100;
			int cy = HIWORD(lParam) - 100;
			//m_pChildWindow->Resize(cx, cy);
			::MoveWindow(m_pChildWindow->GetHWND(), 50, 50, cx, cy, TRUE);
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
#ifndef USE_GDI_CHILD_WINDOW
		::KillTimer(m_hWnd, kAnimationTimer);
#endif
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
	HBRUSH hbrush = m_hbrushFill;
	::FillRect(hdc, &client_rc, hbrush);
}
