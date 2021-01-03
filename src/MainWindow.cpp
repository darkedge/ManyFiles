#include "MainWindow.h"
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "DirectoryNavigationPanel.h"
#include <dwrite.h>
#include <wincodec.h>
#include <d3d11.h>
#include "..\3rdparty\tracy\Tracy.hpp"
#include "Threadpool.h"

#define MJ_WM_SIZE (WM_USER + 0)
#define MJ_WM_TASK (WM_USER + 1)

struct CreateIWICImagingFactoryContext
{
  mj::MainWindow* pMainWindow;
  IWICImagingFactory* pWicFactory;

  static void ExecuteAsync(CreateIWICImagingFactoryContext* pContext)
  {
    ZoneScopedN("CoCreateInstance IWICImagingFactory");
    MJ_ERR_HRESULT(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                                      (IID_PPV_ARGS(&pContext->pWicFactory))));
  }

  static void OnDone(const CreateIWICImagingFactoryContext* pContext)
  {
    svc::ProvideWicFactory(pContext->pWicFactory);
  }
};

void mj::MainWindow::Init(HWND hWnd)
{
  ZoneScoped;
  this->allocator.Init(reinterpret_cast<LPVOID>(Tebibytes(1)));

  // Just run this in the main thread for now. WIC and ShellApi functions depend on this.
  MJ_ERR_HRESULT(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));

  // TODO: Don't use the virtual allocator for ServiceLocator
  svc::Init(&this->allocator);

  this->pDirectoryNavigationPanel = this->allocator.New<DirectoryNavigationPanel>();
  this->pDirectoryNavigationPanel->Init(&this->allocator);

  // Start a bunch of tasks

  // Create WIC factory
  {
    CreateIWICImagingFactoryContext* pContext;
    mj::Task* pTask       = mj::ThreadpoolTaskAlloc(&pContext, CreateIWICImagingFactoryContext::ExecuteAsync,
                                              CreateIWICImagingFactoryContext::OnDone);
    pContext->pMainWindow = this;
    pTask->Submit();
  }

  // This flag adds support for surfaces with a different color channel ordering than the API default.
  // You need it for compatibility with Direct2D.
#ifdef _DEBUG
  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
  UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif

  // This array defines the set of DirectX hardware feature levels this app  supports.
  // The ordering is important and you should  preserve it.
  // Don't forget to declare your app's minimum required feature level in its
  // description.  All apps are assumed to support 9.1 unless otherwise stated.
  D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1, //
                                        D3D_FEATURE_LEVEL_11_0, //
                                        D3D_FEATURE_LEVEL_10_1, //
                                        D3D_FEATURE_LEVEL_10_0, //
                                        D3D_FEATURE_LEVEL_9_3,  //
                                        D3D_FEATURE_LEVEL_9_2,  //
                                        D3D_FEATURE_LEVEL_9_1 };

  // Create the DX11 API device object, and get a corresponding context.
  MJ_UNINITIALIZED ID3D11Device* pD3d11Device;
  MJ_UNINITIALIZED D3D_FEATURE_LEVEL featureLevel;
  {
    ZoneScopedN("D3D11CreateDevice");
    MJ_ERR_HRESULT(::D3D11CreateDevice(nullptr,                  // specify null to use the default adapter
                                       D3D_DRIVER_TYPE_HARDWARE, // Use the fast software driver which loads faster
                                       nullptr,                  //
                                       creationFlags, // optionally set debug and Direct2D compatibility flags
                                       featureLevels, // list of feature levels this app can support
                                       ARRAYSIZE(featureLevels), // number of possible feature levels
                                       D3D11_SDK_VERSION,        //
                                       &pD3d11Device,            // returns the Direct3D device created
                                       &featureLevel,            // returns feature level of device created
                                       nullptr                   // No ID3D11DeviceContext will be returned.
                                       ));
  }
  MJ_DEFER(pD3d11Device->Release());

  // Obtain the underlying DXGI device of the Direct3D11 device.
  {
    MJ_UNINITIALIZED IDXGIDevice1* pDxgiDevice;
    MJ_ERR_HRESULT(pD3d11Device->QueryInterface<IDXGIDevice1>(&pDxgiDevice));
    MJ_DEFER(pDxgiDevice->Release());

    // Ensure that DXGI doesn't queue more than one frame at a time.
    MJ_ERR_HRESULT(pDxgiDevice->SetMaximumFrameLatency(1));

    // Obtain the Direct2D device for 2-D rendering.
    {
      MJ_UNINITIALIZED ID2D1Factory1* pD2d1Factory;
      {
        ZoneScopedN("D2D1CreateFactory");
        MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2d1Factory));
      }
      MJ_DEFER(pD2d1Factory->Release());

      {
        MJ_UNINITIALIZED ID2D1Device* pD2d1Device;
        {
          ZoneScopedN("ID2D1Factory1::CreateDevice");
          MJ_ERR_HRESULT(pD2d1Factory->CreateDevice(pDxgiDevice, &pD2d1Device));
        }
        MJ_DEFER(pD2d1Device->Release());

        // Get Direct2D device's corresponding device context object.
        {
          ZoneScopedN("ID2D1Device::CreateDeviceContext");
          MJ_ERR_HRESULT(pD2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pDeviceContext));
        }
        svc::ProvideD2D1DeviceContext(pDeviceContext);
      }
    }

    // Identify the physical adapter (GPU or card) this device is runs on.
    {
      MJ_UNINITIALIZED IDXGIAdapter* pDxgiAdapter;
      MJ_ERR_HRESULT(pDxgiDevice->GetAdapter(&pDxgiAdapter));
      MJ_DEFER(pDxgiAdapter->Release());

      // Get the factory object that created the DXGI device.
      {
        MJ_UNINITIALIZED IDXGIFactory2* pDxgiFactory;
        MJ_ERR_HRESULT(pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory)));
        MJ_DEFER(pDxgiFactory->Release());

        // Allocate a descriptor.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width                 = 0; // use automatic sizing
        swapChainDesc.Height                = 0;
        swapChainDesc.Format                = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
        swapChainDesc.Stereo                = false;
        swapChainDesc.SampleDesc.Count      = 1; // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality    = 0;
        swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount           = 2; // use double buffering to enable flip
        swapChainDesc.Scaling               = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
        swapChainDesc.Flags                 = 0;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
        fullscreenDesc.Windowed                        = TRUE;

        // Get the final swap chain for this window from the DXGI factory.
        {
          ZoneScopedN("IDXGIFactory2::CreateSwapChainForHwnd");
          MJ_ERR_HRESULT(pDxgiFactory->CreateSwapChainForHwnd(pD3d11Device,    //
                                                              hWnd,            //
                                                              &swapChainDesc,  //
                                                              &fullscreenDesc, //
                                                              nullptr,         // allow on all displays
                                                              &pSwapChain));
        }
      }
    }
  }

  // DirectWrite factory
  {
    MJ_UNINITIALIZED IDWriteFactory* pDwriteFactory;
    {
      ZoneScopedN("DWriteCreateFactory");
      MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                           reinterpret_cast<IUnknown**>(&pDwriteFactory)));
    }
    MJ_DEFER(pDwriteFactory->Release());
    svc::ProvideDWriteFactory(pDwriteFactory);
  }

#if 0
  // WIC
  {
    MJ_ERR_HRESULT(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE));
    MJ_UNINITIALIZED IWICImagingFactory* pWicFactory;
    {
      ZoneScopedN("CoCreateInstance IWICImagingFactory");
      MJ_ERR_HRESULT(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                                        (IID_PPV_ARGS(&pWicFactory))));
    }
    svc::ProvideWicFactory(pWicFactory);
  }
#endif

  // this->allocator.Init(reinterpret_cast<LPVOID>(Tebibytes(1)));
  // this->pDirectoryNavigationPanel = this->allocator.New<DirectoryNavigationPanel>();
  // this->pDirectoryNavigationPanel->Init(&this->allocator);

  // TODO: Put this somewhere else
  {
    ZoneScopedN("GetLogicalDriveStringsW");
    DWORD dw      = ::GetLogicalDriveStringsW(0, nullptr);
    wchar_t* pBuf = this->allocator.New<wchar_t>(dw);
    MJ_DEFER(this->allocator.Free(pBuf));
    dw           = ::GetLogicalDriveStringsW(dw, pBuf);
    wchar_t* ptr = pBuf;
    String str(nullptr);
    while (ptr < pBuf + dw)
    {
      if (*ptr != 0)
      {
        wchar_t* pBegin = ptr;
        while (*ptr != 0)
        {
          ptr++;
        }
        str = String(pBegin, ptr - pBegin);
      }
      ptr++;
    }
  }
}

void mj::MainWindow::Resize()
{
  ZoneScoped;
  // Detach target bitmap
  pDeviceContext->SetTarget(nullptr);

  // Resize
  MJ_ERR_HRESULT(pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0));

  // Create new bitmap from resized buffer
  MJ_UNINITIALIZED IDXGISurface* pBuffer;
  MJ_ERR_HRESULT(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBuffer)));
  MJ_DEFER(pBuffer->Release());

  MJ_UNINITIALIZED ID2D1Bitmap1* pBitmap;
  MJ_ERR_HRESULT(pDeviceContext->CreateBitmapFromDxgiSurface(
      pBuffer,
      D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                              D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE)),
      &pBitmap));
  MJ_DEFER(pBitmap->Release());

  // Attach target bitmap
  pDeviceContext->SetTarget(pBitmap);
}

void mj::MainWindow::Paint()
{
  ZoneScoped;
  this->pDeviceContext->BeginDraw();

  this->pDeviceContext->Clear(D2D1::ColorF(1.0f, 1.0f, 1.0f));
  this->pDirectoryNavigationPanel->Paint();

  MJ_ERR_HRESULT(this->pDeviceContext->EndDraw());
  MJ_ERR_HRESULT(this->pSwapChain->Present(0, 0));
}

void mj::MainWindow::Destroy()
{
  ZoneScoped;
  this->pDirectoryNavigationPanel->Destroy();

  if (pDeviceContext)
  {
    svc::ProvideD2D1DeviceContext(nullptr);
    static_cast<void>(pDeviceContext->Release());
  }
  if (pSwapChain)
  {
    static_cast<void>(pSwapChain->Release());
  }

  ::CoUninitialize();
}

/// <summary>
/// Main WindowProc function
/// </summary>
LRESULT CALLBACK mj::MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  mj::MainWindow* pMainWindow = reinterpret_cast<mj::MainWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_CREATE:
  {
    // Loaded DLLs: uxtheme, combase, msctf, oleaut32

    // Copy the lpParam from CreateWindowEx to this window's user data
    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow       = reinterpret_cast<mj::MainWindow*>(pcs->lpCreateParams);
    MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));

    mj::ThreadpoolInit(hWnd, MJ_WM_TASK);
    pMainWindow->Init(hWnd);

    return ::DefWindowProcW(hWnd, message, wParam, lParam);
  }
  case WM_SIZE:
    // Do not resize while dragging the window border
    pMainWindow->s_Resize = true;
    return 0;
  case MJ_WM_SIZE:
  {
    pMainWindow->Resize();
    MJ_ERR_ZERO(InvalidateRect(hWnd, nullptr, TRUE));
    return 0;
  }
  case WM_DESTROY:
    mj::ThreadpoolDestroy();
    ::PostQuitMessage(0);
    return 0;
  case WM_PAINT:
  {
    MJ_UNINITIALIZED PAINTSTRUCT ps;
    static_cast<void>(::BeginPaint(hWnd, &ps));
    pMainWindow->Paint();
    static_cast<void>(::EndPaint(hWnd, &ps));
    return 0;
  }
  case MJ_WM_TASK:
  {
    mj::ThreadpoolTaskEnd(wParam, lParam);
    return 0;
  }
  default:
    break;
  }

  return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

void mj::MainWindow::Run()
{
  MJ_DEFER(this->Destroy());

  MJ_UNINITIALIZED ATOM cls;
  WNDCLASSEXW wc   = {};
  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.lpfnWndProc   = mj::MainWindow::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = L"Class Name";
  wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
  MJ_ERR_ZERO(cls = ::RegisterClassExW(&wc));
  LPWSTR classAtom = MAKEINTATOM(cls);

  MJ_UNINITIALIZED HWND hWnd;
  {
    ZoneScopedN("CreateWindowExW");
    hWnd = ::CreateWindowExW(0,                                       // Optional window styles.
                             classAtom,                               // Window class
                             L"Window Title",                         // Window text
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,        // Window style
                             CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, // Size and pCurrent
                             nullptr,                                 // Parent window
                             nullptr,                                 // Menu
                             HINST_THISCOMPONENT,                     // Instance handle
                             this);                                   // Additional application data
  }

  // Run the message loop.
  MJ_UNINITIALIZED MSG msg;
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    // If the message is translated, the return value is nonzero.
    // If the message is not translated, the return value is zero.
    static_cast<void>(::TranslateMessage(&msg));

    // The return value specifies the value returned by the window procedure.
    // Although its meaning depends on the message being dispatched,
    // the return value generally is ignored.
    static_cast<void>(::DispatchMessageW(&msg));

    if (s_Resize)
    {
      s_Resize = false;
      MJ_ERR_ZERO(PostMessageW(hWnd, MJ_WM_SIZE, 0, 0));
    }
  }
}
