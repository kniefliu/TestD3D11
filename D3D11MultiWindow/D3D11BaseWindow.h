#pragma once

#include "WindowWnd.h"

#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>
#include <dcomp.h>

#include "D3DConfig.h"

class GDIBaseWindow;
class D3D11BaseWindow : public CWindowWnd {
public:
	D3D11BaseWindow();
	~D3D11BaseWindow() override;

protected:
	LPCTSTR GetWindowClassName() const override;
	UINT GetClassStyle() const override;
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	void OnFinalMessage(HWND hWnd) override;

	HRESULT InitDevice();
	void CleanupDevice();
	virtual void Render();

protected:
	enum { kRenderTimerID = 100 };

protected:
	D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
	D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
	ID3D11Device* g_pd3dDevice = nullptr;
	ID3D11Device1* g_pd3dDevice1 = nullptr;
	ID3D11DeviceContext* g_pImmediateContext = nullptr;
	ID3D11DeviceContext1* g_pImmediateContext1 = nullptr;
	IDXGISwapChain* g_pSwapChain = nullptr;
	IDXGISwapChain1* g_pSwapChain1 = nullptr;
	ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
	ID3D11Texture2D* g_pDepthStencil = nullptr;
	ID3D11DepthStencilView* g_pDepthStencilView = nullptr;
	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11PixelShader* g_pPixelShader = nullptr;
	ID3D11InputLayout* g_pVertexLayout = nullptr;
	ID3D11Buffer* g_pVertexBuffer = nullptr;
	ID3D11Buffer* g_pIndexBuffer = nullptr;
	ID3D11Buffer* g_pCBNeverChanges = nullptr;
	ID3D11Buffer* g_pCBChangeOnResize = nullptr;
	ID3D11Buffer* g_pCBChangesEveryFrame = nullptr;
	ID3D11ShaderResourceView* g_pTextureRV = nullptr;
	ID3D11SamplerState* g_pSamplerLinear = nullptr;
	IDXGIFactory1* m_pDxgiFactory = nullptr;
	IDXGIFactory2* m_pDxgiFactory2 = nullptr;
	IDCompositionDevice* m_pDevice = nullptr;
	IDCompositionTarget* m_pCompositionTarget = nullptr;
	IDCompositionVisual* m_pVisual = nullptr;
	DirectX::XMMATRIX                            g_World;
	DirectX::XMMATRIX                            g_View;
	DirectX::XMMATRIX                            g_Projection;
	DirectX::XMFLOAT4                            g_vMeshColor;
	int m_iLastWidth;
	int m_iLastHeight;
	

protected:
	GDIBaseWindow* m_pChild = nullptr;
};