#pragma once

#include "WindowWnd.h"

#include "D3D11BaseWindow.h"

class GDIBaseWindow : public CWindowWnd {
public:
	GDIBaseWindow();
	~GDIBaseWindow() override;

protected:
	LPCTSTR GetWindowClassName() const override;
	UINT GetClassStyle() const override;
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void OnFinalMessage(HWND hWnd) override;

	virtual void DoPaint(HDC hdc);

protected:
#ifndef USE_GDI_CHILD_WINDOW
	D3D11BaseWindow* m_pChildWindow;
#else
	GDIBaseWindow* m_pChildWindow;
#endif
	HBRUSH m_hbrushFill;
	RECT m_rcCaption;
	RECT m_rcSizeBox;
};