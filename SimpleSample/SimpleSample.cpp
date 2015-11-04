// SimpleSample.cpp : Defines the entry point for the application.
//

#include "stdafx.h"

#include <d3d11.h>
#include <directxmath.h>

#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "Effects.h"
#include "GeometricPrimitive.h"
#include "Model.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "ConstantBuffer.h"

//#include "vsgcapture.h"
#include "SimpleSample.h"

#include "BasicVS.inc"
#include "BasicPS.inc"

#define MAX_LOADSTRING 100
using namespace DirectX;

enum JoinType
{
	Miter,
	Bevel,
	Round,
	FlipBevel
};

enum CapType
{
	Butt,
	Square,
	RoundC
};

  // Vertex struct holding position and color information.
    struct xVertexPositionColorNormal
    {
		xVertexPositionColorNormal() DIRECTX_CTOR_DEFAULT

			xVertexPositionColorNormal(XMFLOAT2 const& position, XMFLOAT4 const& color, XMFLOAT4 const& norm)
          : position(position),
            color(color),
			normal(norm)
        { }

		xVertexPositionColorNormal(FXMVECTOR position, FXMVECTOR color, FXMVECTOR norm)
        {
            XMStoreFloat2(&this->position, position);
            XMStoreFloat4(&this->color, color);
			XMStoreFloat4(&this->normal, norm);
        }

        XMFLOAT2 position;
        XMFLOAT4 color;
		XMFLOAT4 normal;

        static const int InputElementCount = 3;
        static const D3D11_INPUT_ELEMENT_DESC InputElements[InputElementCount];
    };

	struct TriangleElement {
		TriangleElement(uint16_t a_, uint16_t b_, uint16_t c_) : a(a_), b(b_), c(c_) {}
		uint16_t a, b, c;
	};

const D3D11_INPUT_ELEMENT_DESC xVertexPositionColorNormal::InputElements[] =
{
    { "SV_Position", 0, DXGI_FORMAT_R32G32_FLOAT,   0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "COLOR",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL",       0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
};

static_assert( sizeof(xVertexPositionColorNormal) == 40, "Vertex struct/layout mismatch" );

static void xCreateBuffer(_In_ ID3D11Device* device, size_t bufferSize, D3D11_BIND_FLAG bindFlag, _Out_ ID3D11Buffer** pBuffer)
{
    D3D11_BUFFER_DESC desc = { 0 };

    desc.ByteWidth = (UINT)bufferSize;
    desc.BindFlags = bindFlag;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    ThrowIfFailed(
        device->CreateBuffer(&desc, nullptr, pBuffer)
    );

    SetDebugObjectName(*pBuffer, "DirectXTK:PrimitiveBatch");
}

typedef XMFLOAT2 Coordinate;

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND g_hWnd;
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
void Render();
void addGeometry(const std::vector<Coordinate>& vertices);
void addCurrentVertex(const XMVECTOR& currentVertex,
	float flip,
	double distance,
	const XMVECTOR& normal,
	float endLeft,
	float endRight,
	bool round,
	int32_t startVertex,
	std::vector<TriangleElement>& triangleStore);


template <typename elemType, int16_t maxSize = 8192, int16_t bindFlags = D3D11_BIND_VERTEX_BUFFER>
class DXBuffers
{
public:
	DXBuffers(ID3D11DeviceContext* deviceCtx) :mDeviceContext(deviceCtx){
		strideSize = sizeof(elemType);
		maxVertices = maxSize;
		bindFlag = bindFlags;
		//Microsoft::WRL::ComPtr<ID3D11Device> devicePtr(deviceCtx->GetDevice());

		//CreateBuffer(devicePtr, strideSize * maxVertices, bindFlags, vertexBuffer.GetAddressOf());
	}
	~DXBuffers() {}


protected:
	void addElement()
	{
		elements.emplace_back();
	}
	int getIndex()
	{
		return elements.size();
	}
		
	elemType& getAt(size_t index)
	{
		if (index > elements.size())
		{
			throw std::runtime_error("Index out of bounds");
		}
		return elements[index];
	}

	ID3D11Buffer* getBuffer()
	{
		Microsoft::WRL::ComPtr<ID3D11Device> devicePtr;
		mDeviceContext->GetDevice(devicePtr.GetAddressOf());

		if (bindFlag == D3D11_BIND_VERTEX_BUFFER && !vertexBuffer)
		{
			CreateBuffer(devicePtr.Get(), elements.size() * sizeof(elemType), bindFlag, vertexBuffer.GetAddressOf());
		}
		if (bindFlag == D3D11_BIND_INDEX_BUFFER && !indexBuffer)
		{
			
			CreateBuffer(devicePtr.Get(), elements.size() * sizeof(elemType), bindFlag, indexBuffer.GetAddressOf());
		}

		return bindFlag == D3D11_BIND_VERTEX_BUFFER ? vertexBuffer.Get() : indexBuffer.Get();
	}
private:
	static void CreateBuffer(_In_ ID3D11Device* device, size_t bufferSize, uint16_t bindFlag, _Out_ ID3D11Buffer** pBuffer)
	{
		D3D11_BUFFER_DESC desc = { 0 };

		desc.ByteWidth = (UINT)bufferSize;
		desc.BindFlags = bindFlag;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		DirectX::ThrowIfFailed(
			device->CreateBuffer(&desc, nullptr, pBuffer)
			);
	}


	Microsoft::WRL::ComPtr<ID3D11DeviceContext> mDeviceContext;
	Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
	size_t strideSize;
	uint16_t maxVertices;
	uint16_t bindFlag;
	std::vector<elemType> elements;
};

class vertexClassBuffer : public DXBuffers<xVertexPositionColorNormal>
{

public:
	vertexClassBuffer(ID3D11DeviceContext* deviceCtx) : DXBuffers<xVertexPositionColorNormal>(deviceCtx)
	{

	}

	int add(float x, float y, float extx, float exty, int8_t tx, int8_t ty, double distance )
	{
		addElement();
		xVertexPositionColorNormal& elemAdded = getAt(getIndex());

		elemAdded.position.x = static_cast<int16_t>(x * 2) | tx;
		elemAdded.position.y = static_cast<int16_t>(y * 2) | ty;
		XMStoreFloat4(&elemAdded.color, DirectX::Colors::Brown);

		elemAdded.normal.x = 64 * extx;
		elemAdded.normal.y = 64 * exty;
		elemAdded.normal.z = distance / 128;
		elemAdded.normal.w = static_cast<int8_t>(distance) % 128;
		return index();
	}
	int index()
	{
		return getIndex();
	}

	ID3D11Buffer* bind()
	{
		return getBuffer();
	}
};

class indexClassBuffer : public DXBuffers<uint16_t, 8192, D3D11_BIND_INDEX_BUFFER>
{
public:
	indexClassBuffer(ID3D11DeviceContext* deviceCtx) : DXBuffers<uint16_t, 8192, D3D11_BIND_INDEX_BUFFER>(deviceCtx)
	{

	}

	void add(uint16_t indexVal1, uint16_t indexVal2, uint16_t indexVal3)
	{
		addElement();
		getAt(getIndex()) = indexVal1;
		addElement();
		getAt(getIndex()) = indexVal2;
		addElement();
		getAt(getIndex()) = indexVal3;
	}

	ID3D11Buffer* bind()
	{
		return getBuffer();
	}
};

D3D_DRIVER_TYPE                     g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL                   g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11DeviceContext*                g_pImmediateContext = nullptr;
IDXGISwapChain*                     g_pSwapChain = nullptr;
ID3D11RenderTargetView*             g_pRenderTargetView = nullptr;
ID3D11Texture2D*                    g_pDepthStencil = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilView = nullptr;

ID3D11ShaderResourceView*           g_pTextureRV1 = nullptr;
ID3D11ShaderResourceView*           g_pTextureRV2 = nullptr;
ID3D11InputLayout*                  g_pBatchInputLayout = nullptr;

std::unique_ptr<CommonStates>                           g_States;
std::unique_ptr<BasicEffect>                            g_BatchEffect;
std::unique_ptr<EffectFactory>                          g_FXFactory;
std::unique_ptr<GeometricPrimitive>                     g_Shape;
std::unique_ptr<Model>                                  g_Model;
std::unique_ptr<PrimitiveBatch<VertexPositionColor>>    g_Batch;
std::unique_ptr<SpriteBatch>                            g_Sprites;
std::unique_ptr<SpriteFont>                             g_Font;
//std::unique_ptr<VsgDbg> dbg;

Microsoft::WRL::ComPtr<ID3D11VertexShader> VSCode;
Microsoft::WRL::ComPtr<ID3D11PixelShader> PSCode;
Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;

XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_ViewExtrude;
XMMATRIX                            g_Projection;

float	g_pixelRatio;

std::unique_ptr<vertexClassBuffer> vertexBuffer;
std::unique_ptr<indexClassBuffer> triangleElementsBuffer;

__declspec(align(16)) struct SimpleConstants
{
    XMMATRIX matrix;
	XMMATRIX extr_matrix;
	XMFLOAT2A lineWidth;
	float	 blur;
	float	 ratio;
	XMFLOAT4A color;
	XMFLOAT2A pattern_size_a;
	XMFLOAT2A pattern_tl_a;
	XMFLOAT2A pattern_br_a;
	XMFLOAT2A pattern_size_b;
	XMFLOAT2A pattern_tl_b;
	XMFLOAT2A pattern_br_b;
	float fade;
	float opacity;


};


bool operator==(XMFLOAT2 x1, XMFLOAT2 x2)
{
	return x1.x == x2.x && x2.y == x1.y;
}

std::unique_ptr<ConstantBuffer<SimpleConstants>> constBuffers;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMPLESAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

	//// cleanup dbggraph
	//wchar_t tempDir[MAX_PATH];
	//wchar_t filePath[MAX_PATH];

	//if(GetTempPath(MAX_PATH, tempDir) == 0)
	//{
	//	return 0;
	//}

	//swprintf_s(filePath, MAX_PATH, L"%s%s", tempDir, VSG_DEFAULT_RUN_FILENAME);

	//DeleteFile(filePath);


    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

		//dbg.reset(new VsgDbg(true));

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLESAMPLE));


	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Render();
		}
	}


    return (int) msg.wParam;
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
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
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}
	if (FAILED(hr))
		return hr;

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return hr;

	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
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
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

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

	

	
	// Create DirectXTK objects
	g_States.reset(new CommonStates(g_pd3dDevice));
	g_Sprites.reset(new SpriteBatch(g_pImmediateContext));
	g_FXFactory.reset(new EffectFactory(g_pd3dDevice));
	g_Batch.reset(new PrimitiveBatch<VertexPositionColor>(g_pImmediateContext));

	//g_pImmediateContext->OMSetDepthStencilState(g_States->DepthNone(), 0);

	g_BatchEffect.reset(new BasicEffect(g_pd3dDevice));
	g_BatchEffect->SetVertexColorEnabled(true);

	{
		void const* shaderByteCode;
		size_t byteCodeLength;

		//g_BatchEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		hr = g_pd3dDevice->CreateInputLayout(xVertexPositionColorNormal::InputElements,
			xVertexPositionColorNormal::InputElementCount,
			g_BasicVS, sizeof(g_BasicVS),
			&g_pBatchInputLayout);
		if (FAILED(hr))
			return hr;
	}

	constBuffers.reset(new ConstantBuffer<SimpleConstants>(g_pd3dDevice));

	g_Shape = GeometricPrimitive::CreateTeapot(g_pImmediateContext, 4.f, 8, false);
	

	// Initialize the world matrices
	g_World = XMMatrixIdentity();

	// Initialize the view matrix
	XMVECTOR Eye = XMVectorSet(0.0f, 3.0f, -6.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	g_ViewExtrude = XMMatrixOrthographicLH(width, height, 0, 1);

	g_pixelRatio = 1.0;

	g_View = XMMatrixLookAtLH(Eye, At, Up);
	g_BatchEffect->SetWorld(g_World);
	g_BatchEffect->SetView(g_World);

	// Initialize the projection matrix
	g_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, width / (FLOAT)height, 0.01f, 100.0f);

	g_BatchEffect->SetProjection(g_World);

	g_pd3dDevice->CreateVertexShader(g_BasicVS, sizeof(g_BasicVS), nullptr, VSCode.GetAddressOf());
	g_pd3dDevice->CreatePixelShader(g_PSBasicNoFog, sizeof(g_PSBasicNoFog), nullptr, PSCode.GetAddressOf());

	xCreateBuffer(g_pd3dDevice, 6* sizeof(xVertexPositionColorNormal), D3D11_BIND_VERTEX_BUFFER, &mVertexBuffer);

	D3D11_MAPPED_SUBRESOURCE mMappedVertices;

	hr = g_pImmediateContext->Map(mVertexBuffer.Get(), 0 , D3D11_MAP_WRITE_DISCARD, 0, &mMappedVertices);


	xVertexPositionColorNormal vtx1;
	xVertexPositionColorNormal vtx2;
	xVertexPositionColorNormal vtx3;
	xVertexPositionColorNormal vtx4;
	xVertexPositionColorNormal vtx5;
	xVertexPositionColorNormal vtx6;

	//vtx1.position.z = 0.1;
	//vtx2.position.z = 0.1;

	//vtx1.position.w = 1;
	//vtx2.position.w = 1;
	//vtx3.position.w = 1;
	//vtx4.position.w = 1;
	//vtx5.position.w = 1;
	//vtx6.position.w = 1;

	vtx1.position.x = 0.1;
	vtx1.position.y = 0.1;

	vtx2.position.x = 0.1;
	vtx2.position.y = 0.5;

	//vtx3.position.z = 0.1;
	//vtx4.position.z = 0.1;

	vtx3.position.x = 0.5;
	vtx3.position.y = 0.5;

	vtx4.position.x = 0.6;
	vtx4.position.y = 0.6;

	vtx5.position.x = 0.7;
	vtx5.position.y = 0.7;

	vtx6.position.x = 0.7;
	vtx6.position.y = 0.1;


	//vtx5.position.z = 0.1;
	//vtx6.position.z = 0.1;

	 XMStoreFloat4(&vtx1.color, DirectX::Colors::Red);
	 XMStoreFloat4(&vtx2.color, DirectX::Colors::Red);

	 XMStoreFloat4(&vtx3.color, DirectX::Colors::Green);
	 XMStoreFloat4(&vtx4.color, DirectX::Colors::Green);

	 XMStoreFloat4(&vtx5.color, DirectX::Colors::Blue);
	 XMStoreFloat4(&vtx6.color, DirectX::Colors::Blue);

	 //XMStoreFloat4(&vtx1.normal, XMVector2AngleBetweenVectors())


	 memcpy_s(((uint8_t*)mMappedVertices.pData + 0 * sizeof(xVertexPositionColorNormal)),   sizeof(xVertexPositionColorNormal), &vtx1, sizeof(vtx1));
	 memcpy_s(((uint8_t*)mMappedVertices.pData + 1 * sizeof(xVertexPositionColorNormal)),   sizeof(xVertexPositionColorNormal), &vtx2, sizeof(vtx1));
	 memcpy_s(((uint8_t*)mMappedVertices.pData + 2 * sizeof(xVertexPositionColorNormal)),   sizeof(xVertexPositionColorNormal), &vtx3, sizeof(vtx1));
	 memcpy_s(((uint8_t*)mMappedVertices.pData + 3 * sizeof(xVertexPositionColorNormal)),   sizeof(xVertexPositionColorNormal), &vtx4, sizeof(vtx1));
	 memcpy_s(((uint8_t*)mMappedVertices.pData + 4 * sizeof(xVertexPositionColorNormal)), sizeof(xVertexPositionColorNormal), &vtx5, sizeof(vtx1));
	 memcpy_s(((uint8_t*)mMappedVertices.pData + 5 * sizeof(xVertexPositionColorNormal)), sizeof(xVertexPositionColorNormal), &vtx6, sizeof(vtx1));

	 g_pImmediateContext->Unmap(mVertexBuffer.Get(), 0);


	 vertexBuffer.reset(new vertexClassBuffer(g_pImmediateContext));
	 triangleElementsBuffer.reset(new indexClassBuffer(g_pImmediateContext));

	return S_OK;
}

void CleanupDevice()
{
	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pBatchInputLayout) g_pBatchInputLayout->Release();

	if (g_pTextureRV1) g_pTextureRV1->Release();
	if (g_pTextureRV2) g_pTextureRV2->Release();

	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();
	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMPLESAMPLE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SIMPLESAMPLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }
   g_hWnd = hWnd;

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
			case IDM_DEBUGGRAB:
				//dbg->CaptureCurrentFrame();
				break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//--------------------------------------------------------------------------------------
// Render a frame
//--------------------------------------------------------------------------------------
void Render()
{
	// Update our time
	static float t = 0.0f;
	if (g_driverType == D3D_DRIVER_TYPE_REFERENCE)
	{
		t += (float)XM_PI * 0.0125f;
	}
	else
	{
		static uint64_t dwTimeStart = 0;
		uint64_t dwTimeCur = GetTickCount64();
		if (dwTimeStart == 0)
			dwTimeStart = dwTimeCur;
		t = (dwTimeCur - dwTimeStart) / 1000.0f;
	}

	// Rotate cube around the origin
	g_World = XMMatrixRotationY(t);

	//
	// Clear the back buffer
	//
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

	//
	// Clear the depth buffer to 1.0 (max depth)
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11DepthStencilState* dssOld;
	UINT sref;
	g_pImmediateContext->OMGetDepthStencilState(&dssOld, &sref);

	g_pImmediateContext->OMSetDepthStencilState(g_States->DepthNone(), sref);

	// Draw procedurally generated dynamic grid
	const XMVECTORF32 xaxis = { 20.f, 0.f, 0.f };
	const XMVECTORF32 yaxis = { 0.f, 0.f, 20.f };
	//DrawGrid(*g_Batch, xaxis, yaxis, g_XMZero, 20, 20, Colors::Gray);


	// Draw sprite

	// Draw 3D object
	XMMATRIX local = XMMatrixMultiply(g_World, XMMatrixTranslation(-2.f, -2.f, 4.f));
	//g_Shape->Draw(local, g_View, g_Projection, Colors::White, g_pTextureRV1);

	//g_BatchEffect->Apply(g_pImmediateContext);
	g_pImmediateContext->IASetInputLayout(g_pBatchInputLayout);
	g_pImmediateContext->VSSetShader(VSCode.Get(), nullptr, 0);
	g_pImmediateContext->PSSetShader(PSCode.Get(), nullptr, 0);

	UINT vertexStride = (UINT)sizeof(xVertexPositionColorNormal);
	UINT vertexOffset = 0;

	//g_Batch->Begin();



	 //g_Batch->DrawQuad(vtx1, vtx2, vtx3, vtx4);
	float antialiasing = 1;
	float blur = antialiasing;
	float edgeWidth = 0.5;
	float inset = -1;
	float offset = 0;
	float shift = 0;

	if (0 != 0) {
		inset = 0 / 2.0 + antialiasing * 0.5;
		edgeWidth = 1;

		// shift outer lines half a pixel towards the middle to eliminate the crack
		offset = inset - antialiasing / 2.0;
	}

	float outset = offset + edgeWidth + antialiasing / 2.0 + shift;

	SimpleConstants constData;
	constData.matrix = XMMatrixIdentity();
	constData.extr_matrix = g_ViewExtrude;
	constData.ratio = g_pixelRatio;
	constData.blur = 1;
	constData.lineWidth.x = outset;
	constData.lineWidth.y = inset;

	constData.pattern_size_a = XMFLOAT2A( 16, 16);
	constData.pattern_tl_a = XMFLOAT2A( 0,0 );
	constData.pattern_br_a = XMFLOAT2A(0.5, 0.5);
	constData.pattern_size_b = XMFLOAT2A( 16, 16);
	constData.pattern_tl_b = XMFLOAT2A( 0.5,0.5 );
	constData.pattern_br_b = XMFLOAT2A( 1.0,1.0 );
	constData.fade = 1;
	constData.opacity = 1;


	  DirectX::XMStoreFloat4A(&constData.color, DirectX::Colors::Black);

	  constBuffers->SetData(g_pImmediateContext, constData);
	  ID3D11Buffer* buffer = constBuffers->GetBuffer();

        g_pImmediateContext->VSSetConstantBuffers(0, 1, &buffer);
        g_pImmediateContext->PSSetConstantBuffers(0, 1, &buffer);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	g_pImmediateContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &vertexStride, &vertexOffset);

	
	g_pImmediateContext->RSSetState(g_States->CullNone());
	

		g_pImmediateContext->Draw(6, 0);

	//g_Batch->DrawLine(vtx1, vtx2);

	//g_Batch->End();


	XMVECTOR qid = XMQuaternionIdentity();
	const XMVECTORF32 scale = { 0.01f, 0.01f, 0.01f };
	const XMVECTORF32 translate = { 3.f, -2.f, 4.f };
	XMVECTOR rotate = XMQuaternionRotationRollPitchYaw(0, XM_PI / 2.f, XM_PI / 2.f);
	local = XMMatrixMultiply(g_World, XMMatrixTransformation(g_XMZero, qid, scale, g_XMZero, rotate, translate));
	
	//
	// Present our back buffer to our front buffer
	//
	g_pSwapChain->Present(0, 0);
}


int e1, e2, e3;

void addGeometry(const std::vector<Coordinate>& vertices) {
	const auto len = [&vertices] {
		auto l = vertices.size();
		// If the line has duplicate vertices at the end, adjust length to remove them.
		while (l > 2 && vertices[l - 1] == vertices[l - 2]) {
			l--;
		}
		return l;
	}();

	if (len < 2) {
		// fprintf(stderr, "a line must have at least two vertices\n");
		return;
	}

	const float miterLimit = 1.05f ;

	const XMVECTOR firstVertex = XMLoadFloat2(&vertices.front());
	const XMVECTOR lastVertex = XMLoadFloat2(&vertices[len - 1]);
	const bool closed =  DirectX::XMVector2Equal(firstVertex, lastVertex);

	if (len == 2 && closed) {
		// fprintf(stderr, "a line may not have coincident points\n");
		return;
	}

	const CapType beginCap = CapType::Butt;
	const CapType endCap = closed ? CapType::Butt : CapType::RoundC;

	int8_t flip = 1;
	double distance = 0;
	bool startOfLine = true;


	XMVECTOR currentVertex = g_XMQNaN, prevVertex = g_XMQNaN,
		nextVertex = g_XMQNaN;


	XMVECTOR prevNormal = g_XMQNaN, nextNormal = g_XMQNaN;
	
	// the last three vertices added
	e1 = e2 = e3 = -1;

	if (closed) {
		currentVertex = XMLoadFloat2(&vertices[len - 2]);
		nextNormal = XMVector2Orthogonal(DirectX::XMVectorSubtract(firstVertex, currentVertex));
	}

	const int32_t startVertex = (int32_t)0;
	std::vector<TriangleElement> triangleStore;

	for (size_t i = 0; i < len; ++i) {
		if (closed && i == len - 1) {
			// if the line is closed, we treat the last vertex like the first
			nextVertex = XMLoadFloat2(&vertices[i]);
		}
		else if (i + 1 < len) {
			// just the next vertex
			nextVertex = XMLoadFloat2(&vertices[i + 1]);
		}
		else {
			// there is no next vertex
			nextVertex = g_XMQNaN;
		}

		// if two consecutive vertices exist, skip the current one
		if (!XMVector2IsNaN(nextVertex) &&  XMVector2Equal(XMLoadFloat2(&vertices[i]), nextVertex)) {
			continue;
		}

		if ( XMVectorGetX(nextNormal)!= 0 && XMVectorGetY(nextNormal) != 0) {
			prevNormal = nextNormal;
		}
		if (!XMVector2IsNaN(currentVertex)) {
			prevVertex = currentVertex;
		}

		currentVertex = XMLoadFloat2(&vertices[i]);

		// Calculate how far along the line the currentVertex is
		if (!XMVector2IsNaN(prevVertex))
		{
			distance += XMVector2Length(XMVectorSubtract(firstVertex, currentVertex)).m128_f32[0];
		}

		// Calculate the normal towards the next vertex in this line. In case
		// there is no next vertex, pretend that the line is continuing straight,
		// meaning that we are just using the previous normal.
		

		if (!XMVector2IsNaN(nextVertex))
		{
			nextNormal = XMVector2Orthogonal(DirectX::XMVector2Normalize(DirectX::XMVectorSubtract(nextVertex, currentVertex)));
		}
		else
		{
			nextNormal = prevNormal;
		}

		// If we still don't have a previous normal, this is the beginning of a
		// non-closed line, so we're doing a straight "join".
		if (XMVector2IsNaN(prevNormal)) {
			prevNormal = nextNormal;
		}

		// Determine the normal of the join extrusion. It is the angle bisector
		// of the segments between the previous line and the next line.
		XMVECTOR joinNormal ;
		joinNormal = DirectX::XMVector2Normalize(DirectX::XMVectorAdd(prevNormal, nextNormal));
		

		/*  joinNormal     prevNormal
		*             ?      ?
		*                .________. prevVertex
		*                |
		* nextNormal  ?  |  currentVertex
		*                |
		*     nextVertex !
		*
		*/

		// Calculate the length of the miter (the ratio of the miter to the width).
		// Find the cosine of the angle between the next and join normals
		// using dot product. The inverse of that is the miter length.
		const float cosHalfAngle = XMVectorGetX(joinNormal) * XMVectorGetX(nextNormal) + XMVectorGetY(joinNormal) * XMVectorGetY(nextNormal);
		const float miterLength = 1 / cosHalfAngle;

		// The join if a middle vertex, otherwise the cap
		const bool middleVertex = XMVector2IsNaN(prevVertex) && XMVector2IsNaN(nextVertex);
		JoinType currentJoin = JoinType::Miter;
		const CapType currentCap = XMVector2IsNaN(nextVertex) ? beginCap : endCap;

		if (middleVertex) {
			if (currentJoin == JoinType::Round && miterLength < 1) {
				currentJoin = JoinType::Miter;
			}

			if (currentJoin == JoinType::Miter && miterLength > miterLimit) {
				currentJoin = JoinType::Bevel;
			}

			if (currentJoin == JoinType::Bevel) {
				// The maximum extrude length is 128 / 63 = 2 times the width of the line
				// so if miterLength >= 2 we need to draw a different type of bevel where.
				if (miterLength > 2) {
					currentJoin = JoinType::FlipBevel;
				}

				// If the miterLength is really small and the line bevel wouldn't be visible,
				// just draw a miter join to save a triangle.
				if (miterLength < miterLimit) {
					currentJoin = JoinType::Miter;
				}
			}
		}

		if (middleVertex && currentJoin == JoinType::Miter) {
			joinNormal = XMVectorScale(joinNormal, miterLength);
			addCurrentVertex(currentVertex, flip, distance, joinNormal, 0, 0, false, startVertex,
				triangleStore);

		}
		else if (middleVertex && currentJoin == JoinType::FlipBevel) {
			// miter is too big, flip the direction to make a beveled join

			if (miterLength > 100) {
				// Almost parallel lines
				joinNormal = nextNormal;
			}
			else {
				const float direction = XMVectorGetX(prevNormal) * XMVectorGetY(nextNormal) - XMVectorGetY(prevNormal) * XMVectorGetX(nextNormal) > 0 ? -1 : 1;
				const float bevelLength = miterLength * XMVectorGetX(XMVector2LengthSq(XMVectorAdd(prevNormal , nextNormal))) / XMVectorGetX(XMVector2LengthSq(XMVectorSubtract(prevNormal, XMVectorScale(nextNormal, direction))));

				joinNormal = XMVector2Orthogonal(DirectX::XMVector2Normalize(DirectX::XMVectorScale(joinNormal, bevelLength)));
			}

			addCurrentVertex(currentVertex, flip, distance, joinNormal, 0, 0, false, startVertex,
				triangleStore);
			flip = -flip;

		}
		else if (middleVertex && currentJoin == JoinType::Bevel) {
			const float dir = XMVectorGetX(prevNormal) * XMVectorGetY(nextNormal) - XMVectorGetY(prevNormal) * XMVectorGetX(nextNormal);
			const float offset = -std::sqrt(miterLength * miterLength - 1);
			float offsetA;
			float offsetB;

			if (flip * dir > 0) {
				offsetB = 0;
				offsetA = offset;
			}
			else {
				offsetA = 0;
				offsetB = offset;
			}

			// Close previous segement with bevel
			if (!startOfLine) {
				addCurrentVertex(currentVertex, flip, distance, prevNormal, offsetA, offsetB, false,
					startVertex, triangleStore);
			}

			// Start next segment
			if (!XMVector2IsNaN(nextVertex)) {
				addCurrentVertex(currentVertex, flip, distance, nextNormal, -offsetA, -offsetB,
					false, startVertex, triangleStore);
			}

		}
		else if (!middleVertex && currentCap == CapType::Butt) {
			if (!startOfLine) {
				// Close previous segment with a butt
				addCurrentVertex(currentVertex, flip, distance, prevNormal, 0, 0, false,
					startVertex, triangleStore);
			}

			// Start next segment with a butt
			if (!XMVector2IsNaN(nextVertex)) {
				addCurrentVertex(currentVertex, flip, distance, nextNormal, 0, 0, false,
					startVertex, triangleStore);
			}

		}
		else if (!middleVertex && currentCap == CapType::Square) {
			if (!startOfLine) {
				// Close previous segment with a square cap
				addCurrentVertex(currentVertex, flip, distance, prevNormal, 1, 1, false,
					startVertex, triangleStore);

				// The segment is done. Unset vertices to disconnect segments.
				e1 = e2 = -1;
				flip = 1;
			}

			// Start next segment
			if (!XMVector2IsNaN(nextVertex)) {
				addCurrentVertex(currentVertex, flip, distance, nextNormal, -1, -1, false,
					startVertex, triangleStore);
			}

		}
		else if (middleVertex ? currentJoin == JoinType::Round : currentCap == CapType::RoundC) {
			if (!startOfLine) {
				// Close previous segment with a butt
				addCurrentVertex(currentVertex, flip, distance, prevNormal, 0, 0, false,
					startVertex, triangleStore);

				// Add round cap or linejoin at end of segment
				addCurrentVertex(currentVertex, flip, distance, prevNormal, 1, 1, true, startVertex,
					triangleStore);

				// The segment is done. Unset vertices to disconnect segments.
				e1 = e2 = -1;
				flip = 1;

			}
			else if (beginCap == CapType::RoundC) {
				// Add round cap before first segment
				addCurrentVertex(currentVertex, flip, distance, nextNormal, -1, -1, true,
					startVertex, triangleStore);
			}

			// Start next segment with a butt
			if (!XMVector2IsNaN(nextVertex)) {
				addCurrentVertex(currentVertex, flip, distance, nextNormal, 0, 0, false,
					startVertex, triangleStore);
			}
		}

		startOfLine = false;
	}

	const size_t endVertex = vertexBuffer->index();
	const size_t vertexCount = endVertex - startVertex;

	// Store the triangle/line groups.
	{
		
		for (const auto& triangle : triangleStore) {
			triangleElementsBuffer->add(triangle.a, triangle.b,	triangle.c);
		}

	}
}

void addCurrentVertex(const XMVECTOR& currentVertex,
	float flip,
	double distance,
	const XMVECTOR& normal,
	float endLeft,
	float endRight,
	bool round,
	int32_t startVertex,
	std::vector<TriangleElement>& triangleStore) {
	int8_t tx = round ? 1 : 0;

	XMVECTOR extrude;
	extrude = XMVectorScale(normal , flip);
	if (endLeft)
		XMVectorSubtract(extrude, XMVector2Orthogonal(XMVector2Normalize(DirectX::XMVectorScale(normal, endLeft))));
		//extrude = extrude - (util::perp(normal) * endLeft);
	e3 = (int32_t)vertexBuffer->add(currentVertex.m128_f32[0], currentVertex.m128_f32[0], extrude.m128_f32[0], extrude.m128_f32[1], tx, 0,
		distance) -
		startVertex;
	if (e1 >= 0 && e2 >= 0) {
		triangleStore.emplace_back(e1, e2, e3);
	}
	e1 = e2;
	e2 = e3;

	extrude = normal * (-flip);
	if (endRight)
		XMVectorSubtract(extrude, XMVector2Orthogonal(XMVector2Normalize(DirectX::XMVectorScale(normal, endRight))));
		//extrude = extrude - (util::perp(normal) * endRight);
	e3 = (int32_t)vertexBuffer->add(currentVertex.m128_f32[0], currentVertex.m128_f32[0], extrude.m128_f32[0], extrude.m128_f32[1], tx, 1,
		distance) -
		startVertex;
	if (e1 >= 0 && e2 >= 0) {
		triangleStore.emplace_back(e1, e2, e3);
	}
	e1 = e2;
	e2 = e3;
}