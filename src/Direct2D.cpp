#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"
#include "File.h"
#include "FloatMagic.h"
#include "HexEdit.h"
#include <strsafe.h>
#include "minicrt.h"

static IDWriteFactory* s_pDWriteFactory;
static ID2D1Factory* s_pDirect2DFactory;
static ID2D1DeviceContext* s_pDeviceContext;
static IDXGISwapChain1* s_pSwapChain;
static ID2D1SolidColorBrush* s_pBrush;
static IDWriteTextFormat* s_pTextFormat;

static IDWriteTextLayout* s_pTextLayout;

struct Direct2DInitContext
{
  // In
  HWND hWnd;

  // Out
  IDWriteFactory* pDWriteFactory;
  ID2D1Factory1* pDirect2DFactory;
  ID2D1DeviceContext* pDeviceContext;
  IDXGISwapChain1* pSwapChain;
  ID2D1SolidColorBrush* pBrush;
  IDWriteTextFormat* pTextFormat;
};

struct Direct2DInitContextAssert
{
  static_assert(sizeof(Direct2DInitContext) <= 64);
};

static void Direct2DInitJoin(const mj::TaskContext* pTaskContext)
{
  const Direct2DInitContext* pContext = reinterpret_cast<const Direct2DInitContext*>(pTaskContext);

  s_pDWriteFactory   = pContext->pDWriteFactory;
  s_pDirect2DFactory = pContext->pDirect2DFactory;
  s_pDeviceContext   = pContext->pDeviceContext;
  s_pSwapChain       = pContext->pSwapChain;
  s_pBrush           = pContext->pBrush;
  s_pTextFormat      = pContext->pTextFormat;
}

static void Direct2DInitAsync(mj::TaskContext* pTaskContext)
{
  Direct2DInitContext* pContext = reinterpret_cast<Direct2DInitContext*>(pTaskContext);

  // DirectWrite factory
  MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(&pContext->pDWriteFactory)));

  // Direct2D factory
  MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pContext->pDirect2DFactory));

  // Note: D3D11_CREATE_DEVICE_BGRA_SUPPORT is required
  // for Direct2D interoperability with Direct3D resources.
  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  MJ_UNINITIALIZED mj::DeferRelease<ID3D11Device> pDevice;
  MJ_ERR_HRESULT(::D3D11CreateDevice(nullptr,                  //
                                     D3D_DRIVER_TYPE_HARDWARE, //
                                     nullptr,                  //
                                     flags,                    //
                                     nullptr, 0,               //
                                     D3D11_SDK_VERSION,        //
                                     &pDevice,                 //
                                     nullptr,                  //
                                     nullptr));

  MJ_UNINITIALIZED mj::DeferRelease<IDXGIDevice> pDxgiDevice;
  MJ_ERR_HRESULT(pDevice->QueryInterface(__uuidof(pDxgiDevice), reinterpret_cast<void**>(&pDxgiDevice)));

  MJ_UNINITIALIZED mj::DeferRelease<IDXGIAdapter> pAdapter;
  MJ_ERR_HRESULT(pDxgiDevice->GetAdapter(&pAdapter));

  MJ_UNINITIALIZED mj::DeferRelease<IDXGIFactory2> pFactory;
  MJ_ERR_HRESULT(pAdapter->GetParent(__uuidof(pFactory), reinterpret_cast<void**>(&pFactory)));

  DXGI_SWAP_CHAIN_DESC1 sd = {};
  sd.Format                = DXGI_FORMAT_B8G8R8A8_UNORM;
  sd.SampleDesc.Count      = 1;
  sd.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.BufferCount           = 2;

  MJ_ERR_HRESULT(
      pFactory->CreateSwapChainForHwnd(pDevice, pContext->hWnd, &sd, nullptr, nullptr, &pContext->pSwapChain));

  MJ_UNINITIALIZED mj::DeferRelease<ID2D1Device> pD2DDevice;
  MJ_ERR_HRESULT(pContext->pDirect2DFactory->CreateDevice(pDxgiDevice, &pD2DDevice));

  MJ_ERR_HRESULT(pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pContext->pDeviceContext));

  MJ_UNINITIALIZED mj::DeferRelease<IDXGISurface> pSurface;
  MJ_ERR_HRESULT(pContext->pSwapChain->GetBuffer(0, __uuidof(pSurface), reinterpret_cast<void**>(&pSurface)));

  const D2D1_BITMAP_PROPERTIES1 bp =
      D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                              D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

  MJ_UNINITIALIZED mj::DeferRelease<ID2D1Bitmap1> pBitmap;
  MJ_ERR_HRESULT(pContext->pDeviceContext->CreateBitmapFromDxgiSurface(pSurface, &bp, &pBitmap));
  pContext->pDeviceContext->SetTarget(pBitmap);

  // Init-once stuff
  MJ_ERR_HRESULT(pContext->pDWriteFactory->CreateTextFormat(L"Consolas",                // Font name
                                                            nullptr,                    // Font collection
                                                            DWRITE_FONT_WEIGHT_NORMAL,  // Font weight
                                                            DWRITE_FONT_STYLE_NORMAL,   // Font style
                                                            DWRITE_FONT_STRETCH_NORMAL, // Font stretch
                                                            16.0f,                      // Font size
                                                            L"",                        // Locale name
                                                            &pContext->pTextFormat));

  MJ_ERR_HRESULT(pContext->pDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pContext->pBrush));
}

void mj::Direct2DInit(HWND hwnd)
{
  MJ_EXIT_NULL(hwnd);

  mj::Task* pTask = mj::ThreadpoolTaskAlloc(::Direct2DInitAsync, ::Direct2DInitJoin);

  Direct2DInitContext* pContext = reinterpret_cast<Direct2DInitContext*>(pTask->pContext);
  pContext->hWnd                          = hwnd;

  pTask->Submit();
}

/// <summary>
/// Main thread
/// </summary>
void mj::RenderHexEditBuffer()
{
  const mj::WideString string = mj::HexEditGetBuffer();
  if (string.pString && s_pDWriteFactory && s_pDeviceContext)
  {
    if (s_pTextLayout)
    {
      s_pTextLayout->Release();
    }
    MJ_ERR_HRESULT(s_pDWriteFactory->CreateTextLayout(string.pString, string.length, s_pTextFormat, 1000.0f, 1000.0f,
                                                      &s_pTextLayout));
  }
}

UINT mj::GetRenderedTextHeight()
{
  UINT height = 0;

  if (s_pTextLayout)
  {
    MJ_UNINITIALIZED DWRITE_TEXT_METRICS metrics;
    MJ_ERR_HRESULT(s_pTextLayout->GetMetrics(&metrics));
    height = static_cast<UINT>(metrics.height);
  }

  return height;
}

void mj::Direct2DDraw(int y)
{
  if (s_pDeviceContext && s_pTextLayout && s_pSwapChain)
  {
    s_pDeviceContext->BeginDraw();

    // Note MJ: Clear the screen once to a different color to get a sense of the loading time
    s_pDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // This can be very slow, once the number of characters goes into the millions.
    // Even in release mode!
    s_pDeviceContext->DrawTextLayout(D2D1::Point2F(0.0f, static_cast<FLOAT>(-y)), s_pTextLayout, s_pBrush,
                                     D2D1_DRAW_TEXT_OPTIONS_CLIP);

    MJ_ERR_HRESULT(s_pDeviceContext->EndDraw());

    MJ_ERR_HRESULT(s_pSwapChain->Present(1, 0));
  }
}

/// <summary>
/// Resizes the render target.
/// </summary>
void mj::Direct2DOnSize(WORD width, WORD height)
{
  if (s_pSwapChain)
  {
    MJ_UNINITIALIZED D2D1_SIZE_U size;
    size.width  = width;
    size.height = height;
    // s_pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
  }
}

void mj::Direct2DDestroy()
{
  MJ_SAFE_RELEASE(s_pDWriteFactory);
  MJ_SAFE_RELEASE(s_pDirect2DFactory);
  MJ_SAFE_RELEASE(s_pDeviceContext);
  MJ_SAFE_RELEASE(s_pSwapChain);
  MJ_SAFE_RELEASE(s_pBrush);
  MJ_SAFE_RELEASE(s_pTextFormat);

  MJ_SAFE_RELEASE(s_pTextLayout);
}
