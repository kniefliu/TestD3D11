#include "D3D11BaseWindow.h"

#include "DDSTextureLoader.h"

#include <string>
#include <dwmapi.h>

#include "GDIBaseWindow.h"

std::wstring GetLastErrorAsString()
{
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::wstring(); //No error message has been recorded

	LPWSTR messageBuffer = nullptr;
	size_t size = ::FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

	std::wstring message(messageBuffer, size);

	//Free the buffer.
	::LocalFree(messageBuffer);

	return message;
}

using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------
struct SimpleVertex
{
	XMFLOAT3 Pos;
	XMFLOAT2 Tex;
};

struct CBNeverChanges
{
	XMMATRIX mView;
};

struct CBChangeOnResize
{
	XMMATRIX mProjection;
};

struct CBChangesEveryFrame
{
	XMMATRIX mWorld;
	XMFLOAT4 vMeshColor;
};

//--------------------------------------------------------------------------------------
// Helper for compiling shaders with D3DCompile
//
// With VS 11, we could load up prebuilt .cso files instead...
//--------------------------------------------------------------------------------------
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;

	// Disable optimizations to further improve shader debugging
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			OutputDebugStringA(reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()));
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}

D3D11BaseWindow::D3D11BaseWindow()
	: g_vMeshColor(0.7f, 0.7f, 0.7f, 1.0f)
	, m_iLastWidth(0)
	, m_iLastHeight(0)
{

}
D3D11BaseWindow::~D3D11BaseWindow()
{

}

LPCTSTR D3D11BaseWindow::GetWindowClassName() const
{
	return L"D3D11BaseWindow";
}
UINT D3D11BaseWindow::GetClassStyle() const
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
LRESULT D3D11BaseWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
	{
#if D3D11BASE_HAS_CHILD
		m_pChild = new GDIBaseWindow();
		m_pChild->Create(m_hWnd, L"D3D11-Child", UI_WNDSTYLE_CHILD, 0);
#endif
		//DisableNCRendering(m_hWnd);
		InitDevice();
		::SetTimer(m_hWnd, kRenderTimerID, 15, 0);
		break;
	}
	case WM_SIZE:
	{
		InitDevice();
		if (m_pChild) {
			int cx = LOWORD(lParam) - 200;
			int cy = HIWORD(lParam) - 200;
			//m_pChildWindow->Resize(cx, cy);
			::MoveWindow(m_pChild->GetHWND(), 50, 50, cx, cy, TRUE);
		}
		break;
	}
	case WM_ERASEBKGND:
	{
		break;// return 1;
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RECT rc;
		::GetClientRect(m_hWnd, &rc);
		HDC hdc = ::BeginPaint(m_hWnd, &ps);
		//::SendMessage(::GetParent(m_hWnd), WM_PAINT, 0, 0);
		//::FillRect(hdc, &rc, (HBRUSH)::GetStockObject(WHITE_BRUSH));
		Render();
		::EndPaint(m_hWnd, &ps);
		return 1;
	}
	case WM_TIMER:
	{
		if (wParam == kRenderTimerID) {
			Render();
			//if (m_pChild) {
			//	::InvalidateRect(m_pChild->GetHWND(), NULL, TRUE);
			//}
		}
		break;
	}
	case WM_DESTROY:
	{
		::KillTimer(m_hWnd, kRenderTimerID);
		CleanupDevice();
		PostQuitMessage(0);
		break;
	}
	default:
		break;
	}
	return CWindowWnd::HandleMessage(uMsg, wParam, lParam);
}

void D3D11BaseWindow::OnFinalMessage(HWND hWnd)
{
	delete this;
}

HRESULT D3D11BaseWindow::InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(m_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	if (m_iLastWidth == width && m_iLastHeight == height) {
		return hr;
	}

	if (!g_pd3dDevice) {
		UINT createDeviceFlags = 0;
#ifdef _DEBUG
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_DRIVER_TYPE driverTypes[] =
		{
			D3D_DRIVER_TYPE_HARDWARE,
			D3D_DRIVER_TYPE_WARP,
			D3D_DRIVER_TYPE_REFERENCE,
		};
		UINT numDriverTypes = ARRAYSIZE(driverTypes);

		D3D_FEATURE_LEVEL featureLevels[] =
		{
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
		};
		UINT numFeatureLevels = ARRAYSIZE(featureLevels);

		for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
		{
			g_driverType = driverTypes[driverTypeIndex];
			hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
				D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);

			if (hr == E_INVALIDARG)
			{
				// DirectX 11.0 platforms will not recognize D3D_FEATURE_LEVEL_11_1 so we need to retry without it
				hr = D3D11CreateDevice(nullptr, g_driverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
					D3D11_SDK_VERSION, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
			}

			if (SUCCEEDED(hr))
				break;
		}
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"D3D11CreateDevice Error", MB_OK);
			return hr;
		}
	}

	// Obtain DXGI factory from device (since we used nullptr for pAdapter above)
	if (!m_pDxgiFactory)
	{
		IDXGIDevice* dxgiDevice = nullptr;
		hr = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
		if (SUCCEEDED(hr))
		{
			IDXGIAdapter* adapter = nullptr;
			hr = dxgiDevice->GetAdapter(&adapter);
			if (SUCCEEDED(hr))
			{
				hr = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&m_pDxgiFactory));
				adapter->Release();
			}
			dxgiDevice->Release();
		}

		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"ID3D11Device  QueryInterface IDXGIDevice", MB_OK);
			return hr;
		}

		// Create swap chain
		hr = m_pDxgiFactory->QueryInterface(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_pDxgiFactory2));
		if (m_pDxgiFactory2)
		{
			// DirectX 11.1 or later
			hr = g_pd3dDevice->QueryInterface(__uuidof(ID3D11Device1), reinterpret_cast<void**>(&g_pd3dDevice1));
			if (SUCCEEDED(hr))
			{
				(void)g_pImmediateContext->QueryInterface(__uuidof(ID3D11DeviceContext1), reinterpret_cast<void**>(&g_pImmediateContext1));
			}
			else {
				std::wstring errMsg = GetLastErrorAsString();
				::MessageBox(nullptr,
					errMsg.c_str(), L"QueryInterface ID3D11DeviceContext1", MB_OK);
			}
		}
		// Note this tutorial doesn't handle full-screen swapchains so we block the ALT+ENTER shortcut
		m_pDxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);
	}
	if (!m_pDxgiFactory) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"IDXGIFactory1 Null", MB_OK);
		return hr;
	}
	if (g_pSwapChain1) {
		g_pSwapChain1->Release();
		g_pSwapChain1 = nullptr;
	}
	if (g_pSwapChain) {
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (m_pDxgiFactory2) {
#if USE_D3D11_FLIP
		if (!m_pVisual) {
			HMODULE dcomp = ::LoadLibrary(TEXT("dcomp.dll"));
			if (!dcomp)
			{
				return E_INVALIDARG;
			}

			typedef HRESULT(WINAPI * PFN_DCOMPOSITION_CREATE_DEVICE)(
				IDXGIDevice * dxgiDevice, REFIID iid, void** dcompositionDevice);
			PFN_DCOMPOSITION_CREATE_DEVICE createDComp =
				reinterpret_cast<PFN_DCOMPOSITION_CREATE_DEVICE>(
					GetProcAddress(dcomp, "DCompositionCreateDevice"));
			if (!createDComp)
			{
				return E_INVALIDARG;
			}

			if (!m_pDevice)
			{
				IDXGIDevice* dxgiDevice = nullptr;
				HRESULT result = g_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
				if (SUCCEEDED(result))
				{
					HRESULT result = createDComp(dxgiDevice, __uuidof(IDCompositionDevice),
						reinterpret_cast<void**>(&m_pDevice));
					dxgiDevice->Release();

					if (FAILED(result))
					{
						return result;
					}
				}

				if (!m_pCompositionTarget)
				{
					HRESULT result =
						m_pDevice->CreateTargetForHwnd(m_hWnd, TRUE, &m_pCompositionTarget);
					if (FAILED(result))
					{
						return result;
					}
				}

				if (!m_pVisual)
				{
					HRESULT result = m_pDevice->CreateVisual(&m_pVisual);
					if (FAILED(result))
					{
						return result;
					}
				}
			}
		}
		DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Stereo = FALSE;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.BufferCount = 2;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
		sd.Flags = 0;

		hr = m_pDxgiFactory2->CreateSwapChainForComposition(g_pd3dDevice, &sd, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}
		m_pVisual->SetContent(g_pSwapChain1);
		m_pCompositionTarget->SetRoot(m_pVisual);
#else
		DXGI_SWAP_CHAIN_DESC1 sd = { 0 };
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.Stereo = FALSE;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_SHADER_INPUT;
		sd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
		sd.BufferCount = 1;
		sd.Scaling = DXGI_SCALING_STRETCH;
		sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
		sd.Flags = 0;

		hr = m_pDxgiFactory2->CreateSwapChainForHwnd(g_pd3dDevice, m_hWnd, &sd, nullptr, nullptr, &g_pSwapChain1);
		//hr = m_pDxgiFactory2->CreateSwapChainForComposition(g_pd3dDevice, &sd, nullptr, &g_pSwapChain1);
		if (SUCCEEDED(hr))
		{
			hr = g_pSwapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&g_pSwapChain));
		}
#endif
	}
	else
	{
		// DirectX 11.0 systems
		DXGI_SWAP_CHAIN_DESC sd = {};
		sd.BufferCount = 1;
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
		sd.OutputWindow = m_hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;

		hr = m_pDxgiFactory->CreateSwapChain(g_pd3dDevice, &sd, &g_pSwapChain);
	}

	if (FAILED(hr)) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"CreateSwapChain Failed", MB_OK);
		return hr;
	}

	if (g_pRenderTargetView) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = nullptr;
	}

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr)) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"SwapChain GetBuffer Failed", MB_OK);
		return hr;
	}

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr)) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"CreateRenderTargetView Failed", MB_OK);
		return hr;
	}

	if (g_pDepthStencil) {
		g_pDepthStencil->Release();
		g_pDepthStencil = nullptr;
	}
	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = width;
	descDepth.Height = height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr)) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"DepthStencil CreateTexture2D Failed", MB_OK);
		return hr;
	}

	if (g_pDepthStencilView) {
		g_pDepthStencilView->Release();
		g_pDepthStencilView = nullptr;
	}
	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr)) {
		std::wstring errMsg = GetLastErrorAsString();
		::MessageBox(nullptr,
			errMsg.c_str(), L"CreateDepthStencilView Failed", MB_OK);
		return hr;
	}

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Setup the viewport
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)width;
	vp.Height = (FLOAT)height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);

	if (!g_pVertexShader) {
		// Compile the vertex shader
		ID3DBlob* pVSBlob = nullptr;

		std::wstring strD3DResPath;
#ifdef D3D_RES_PATH
		char szAnisDuiResPath[MAX_PATH] = { 0 };
		strcpy_s(szAnisDuiResPath, D3D_RES_PATH);
		TCHAR szDuiResPath[MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_ACP, 0, szAnisDuiResPath, strlen(szAnisDuiResPath), szDuiResPath, MAX_PATH - 1);
		strD3DResPath = szDuiResPath;
#endif
		std::wstring fxPath = strD3DResPath + L"Test.fx";
		hr = CompileShaderFromFile(fxPath.c_str(), "VS", "vs_4_0", &pVSBlob);
		if (FAILED(hr))
		{
			fxPath = L"Test.fx";
			hr = CompileShaderFromFile(fxPath.c_str(), "VS", "vs_4_0", &pVSBlob);
			if (FAILED(hr))
			{
				::MessageBox(nullptr,
					L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
				return hr;
			}
		}

		// Create the vertex shader
		hr = g_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &g_pVertexShader);
		if (FAILED(hr))
		{
			pVSBlob->Release();
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateVertexShader Failed", MB_OK);
			return hr;
		}

		// Define the input layout
		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		UINT numElements = ARRAYSIZE(layout);

		// Create the input layout
		hr = g_pd3dDevice->CreateInputLayout(layout, numElements, pVSBlob->GetBufferPointer(),
			pVSBlob->GetBufferSize(), &g_pVertexLayout);
		pVSBlob->Release();
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateInputLayout Failed", MB_OK);
			return hr;
		}

		// Set the input layout
		g_pImmediateContext->IASetInputLayout(g_pVertexLayout);

		// Compile the pixel shader
		ID3DBlob* pPSBlob = nullptr;
		hr = CompileShaderFromFile(fxPath.c_str(), "PS", "ps_4_0", &pPSBlob);
		if (FAILED(hr))
		{
			::MessageBox(nullptr,
				L"The FX file cannot be compiled.  Please run this executable from the directory that contains the FX file.", L"Error", MB_OK);
			return hr;
		}

		// Create the pixel shader
		hr = g_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &g_pPixelShader);
		pPSBlob->Release();
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreatePixelShader Failed", MB_OK);
			return hr;
		}

		// Create vertex buffer
		SimpleVertex vertices[] =
		{
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },

			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT2(1.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT2(0.0f, 0.0f) },

			{ XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f) },
			{ XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT2(0.0f, 1.0f) },
			{ XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(0.0f, 0.0f) },
			{ XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 0.0f) },
		};

		D3D11_BUFFER_DESC bd = {};
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(SimpleVertex) * 24;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;

		D3D11_SUBRESOURCE_DATA InitData = {};
		InitData.pSysMem = vertices;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pVertexBuffer);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateBuffer VertexBuffer Failed", MB_OK);
			return hr;
		}

		// Set vertex buffer
		UINT stride = sizeof(SimpleVertex);
		UINT offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

		// Create index buffer
		// Create vertex buffer
		WORD indices[] =
		{
			3,1,0,
			2,1,3,

			6,4,5,
			7,4,6,

			11,9,8,
			10,9,11,

			14,12,13,
			15,12,14,

			19,17,16,
			18,17,19,

			22,20,21,
			23,20,22
		};

		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(WORD) * 36;
		bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bd.CPUAccessFlags = 0;
		InitData.pSysMem = indices;
		hr = g_pd3dDevice->CreateBuffer(&bd, &InitData, &g_pIndexBuffer);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateBuffer IndexBuffer Failed", MB_OK);
			return hr;
		}

		// Set index buffer
		g_pImmediateContext->IASetIndexBuffer(g_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

		// Set primitive topology
		g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Create the constant buffers
		bd.Usage = D3D11_USAGE_DEFAULT;
		bd.ByteWidth = sizeof(CBNeverChanges);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = 0;
		hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBNeverChanges);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateBuffer CBNeverChanges Failed", MB_OK);
			return hr;
		}

		bd.ByteWidth = sizeof(CBChangeOnResize);
		hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangeOnResize);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateBuffer CBChangeOnResize Failed", MB_OK);
			return hr;
		}

		bd.ByteWidth = sizeof(CBChangesEveryFrame);
		hr = g_pd3dDevice->CreateBuffer(&bd, nullptr, &g_pCBChangesEveryFrame);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateBuffer CBChangesEveryFrame Failed", MB_OK);
			return hr;
		}

		// Load the Texture
		std::wstring texPath = strD3DResPath + L"seafloor.dds";
		hr = CreateDDSTextureFromFile(g_pd3dDevice, texPath.c_str(), nullptr, &g_pTextureRV);
		if (FAILED(hr)) {
			texPath = L"seafloor.dds";
			hr = CreateDDSTextureFromFile(g_pd3dDevice, texPath.c_str(), nullptr, &g_pTextureRV);
			if (FAILED(hr)) {
				std::wstring errMsg = GetLastErrorAsString();
				::MessageBox(nullptr,
					errMsg.c_str(), L"CreateDDSTextureFromFile Failed", MB_OK);
				return hr;
			}
		}

		// Create the sample state
		D3D11_SAMPLER_DESC sampDesc = {};
		sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampDesc.MinLOD = 0;
		sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
		hr = g_pd3dDevice->CreateSamplerState(&sampDesc, &g_pSamplerLinear);
		if (FAILED(hr)) {
			std::wstring errMsg = GetLastErrorAsString();
			::MessageBox(nullptr,
				errMsg.c_str(), L"CreateSamplerState Failed", MB_OK);
			return hr;
		}
	}

	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -6.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	g_View = XMMatrixLookAtLH(Eye, At, Up);

	CBNeverChanges cbNeverChanges;
	cbNeverChanges.mView = XMMatrixTranspose(g_View);
	g_pImmediateContext->UpdateSubresource(g_pCBNeverChanges, 0, nullptr, &cbNeverChanges, 0, 0);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f);

	CBChangeOnResize cbChangesOnResize;
	cbChangesOnResize.mProjection = XMMatrixTranspose(g_Projection);
	g_pImmediateContext->UpdateSubresource(g_pCBChangeOnResize, 0, nullptr, &cbChangesOnResize, 0, 0);

	return S_OK;
}
void D3D11BaseWindow::CleanupDevice()
{
	if (m_pDxgiFactory2) m_pDxgiFactory2->Release();
	if (m_pDxgiFactory) m_pDxgiFactory->Release();

	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSamplerLinear) g_pSamplerLinear->Release();
	if (g_pTextureRV) g_pTextureRV->Release();
	if (g_pCBNeverChanges) g_pCBNeverChanges->Release();
	if (g_pCBChangeOnResize) g_pCBChangeOnResize->Release();
	if (g_pCBChangesEveryFrame) g_pCBChangesEveryFrame->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();
	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain1) g_pSwapChain1->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext1) g_pImmediateContext1->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice1) g_pd3dDevice1->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}
void D3D11BaseWindow::Render()
{
	if (!g_pImmediateContext)
		return;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static ULONGLONG timeStart = 0;
		ULONGLONG timeCur = GetTickCount64();
		if (timeStart == 0)
			timeStart = timeCur;
		t = (timeCur - timeStart) / 1000.0f;
	}

	// Rotate cube around the origin
	g_World = XMMatrixRotationY(t);

	// Modify the color
	g_vMeshColor.x = (sinf(t * 1.0f) + 1.0f) * 0.5f;
	g_vMeshColor.y = (cosf(t * 3.0f) + 1.0f) * 0.5f;
	g_vMeshColor.z = (sinf(t * 5.0f) + 1.0f) * 0.5f;

	//
	// Clear the back buffer
	//
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MediumSpringGreen);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//
	// Update variables that change once per frame
	//
	CBChangesEveryFrame cb;
	cb.mWorld = XMMatrixTranspose(g_World);
	cb.vMeshColor = g_vMeshColor;
	g_pImmediateContext->UpdateSubresource(g_pCBChangesEveryFrame, 0, nullptr, &cb, 0, 0);

	//
	// Render the cube
	//
	g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pCBNeverChanges);
	g_pImmediateContext->VSSetConstantBuffers(1, 1, &g_pCBChangeOnResize);
	g_pImmediateContext->VSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
	g_pImmediateContext->PSSetConstantBuffers(2, 1, &g_pCBChangesEveryFrame);
	g_pImmediateContext->PSSetShaderResources(0, 1, &g_pTextureRV);
	g_pImmediateContext->PSSetSamplers(0, 1, &g_pSamplerLinear);
	g_pImmediateContext->DrawIndexed(36, 0, 0);

	//
	// Present our back buffer to our front buffer
	//
	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	RECT rect = { 0, 0, (rc.right - rc.left), (rc.bottom - rc.top) };
	DXGI_PRESENT_PARAMETERS params = { 1, &rect, nullptr, nullptr };
	if (g_pSwapChain1) {
		g_pSwapChain1->Present1(1, 0, &params);
	}
	else if (g_pSwapChain) {
		g_pSwapChain->Present(1, 0);
	}
	if (m_pDevice)
	{
		m_pDevice->Commit();
	}
}
