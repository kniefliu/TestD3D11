#pragma once

#include "WindowWnd.h"
#include "D3DConfig.h"
#include "D3D11BaseWindow.h"
#include "WndShadow.h"

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
	enum { kAnimationTimer = 100 };
protected:
#if USE_GDI_CHILD_WINDOW
	GDIBaseWindow* m_pChildWindow;
#else
	D3D11BaseWindow* m_pChildWindow;
#endif
	CWndShadow* m_pShadowWindow;
	HBRUSH m_hbrushFill;
	HBRUSH m_hbrushBackground;
	RECT m_rcCaption;
	RECT m_rcSizeBox;
};