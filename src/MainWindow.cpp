#include "MainWindow.h"
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "DirectoryNavigationPanel.h"
#include <dwrite.h>
#include <wincodec.h>
#include "../3rdparty/tracy/Tracy.hpp"
#include "Threadpool.h"
#include "ResourcesD2D1.h"
#include <dwmapi.h>

struct CreateIWICImagingFactoryContext : public mj::Task
{
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;
  MJ_UNINITIALIZED IWICImagingFactory* pWicFactory;

  virtual void Execute() override
  {
    ZoneScoped;

    {
      ZoneScopedN("CoInitializeEx");
      MJ_ERR_HRESULT(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
    }

    {
      ZoneScopedN("CoCreateInstance");
      // TODO: Can cause exception c0000374?
      MJ_ERR_HRESULT(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                                        (IID_PPV_ARGS(&this->pWicFactory))));
    }
  }

  virtual void OnDone() override
  {
    svc::ProvideWicFactory(this->pWicFactory);
  }
};

struct CreateID2D1RenderTargetContext : public mj::Task
{
  // In
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;
  MJ_UNINITIALIZED RECT rect;

  // Out
  MJ_UNINITIALIZED ID2D1RenderTarget* pRenderTarget;
  MJ_UNINITIALIZED IDXGISwapChain1* pSwapChain;
  MJ_UNINITIALIZED IDCompositionDevice* dcompDevice;

  virtual void Execute() override
  {
    ZoneScoped;
#if 0
    // Create a DC render target.
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0, 0,
        D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    MJ_UNINITIALIZED ID2D1Factory* pFactory;
    MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory));
    MJ_ERR_HRESULT(pFactory->CreateDCRenderTarget(&props, &this->pRenderTarget));
    pFactory->Release();
#else
#ifdef _DEBUG
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
    UINT dxgiFlags     = DXGI_CREATE_FACTORY_DEBUG;
#else
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    UINT dxgiFlags     = 0;
#endif
    MJ_UNINITIALIZED ID3D11Device* pD3d11Device;
    MJ_ERR_HRESULT(::D3D11CreateDevice(nullptr,                  // specify null to use the default adapter
                                       D3D_DRIVER_TYPE_HARDWARE, // Use the fast software driver which loads faster
                                       0,                        //
                                       creationFlags,     // optionally set debug and Direct2D compatibility flags
                                       nullptr,           // list of feature levels this app can support
                                       0,                 // number of possible feature levels
                                       D3D11_SDK_VERSION, //
                                       &pD3d11Device,     // returns the Direct3D device created
                                       nullptr,           // returns feature level of device created
                                       nullptr            // No ID3D11DeviceContext will be returned.
                                       ));
    MJ_DEFER(pD3d11Device->Release());

    // Obtain the underlying DXGI device of the Direct3D11 device.
    {
      MJ_UNINITIALIZED IDXGIDevice1* pDxgiDevice;
      MJ_ERR_HRESULT(pD3d11Device->QueryInterface<IDXGIDevice1>(&pDxgiDevice));
      MJ_DEFER(pDxgiDevice->Release());

      // Ensure that DXGI doesn't queue more than one frame at a time.
      // MJ_ERR_HRESULT(pDxgiDevice->SetMaximumFrameLatency(1));

      MJ_UNINITIALIZED ID2D1DeviceContext* pDeviceContext;
      // Obtain the Direct2D device for 2-D rendering.
      {
        MJ_UNINITIALIZED ID2D1Factory1* pD2d1Factory;
        MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2d1Factory));
        MJ_DEFER(pD2d1Factory->Release());

        {
          MJ_UNINITIALIZED ID2D1Device* pD2d1Device;
          MJ_ERR_HRESULT(pD2d1Factory->CreateDevice(pDxgiDevice, &pD2d1Device));
          MJ_DEFER(pD2d1Device->Release());

          // Get Direct2D device's corresponding device context object.
          MJ_ERR_HRESULT(pD2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pDeviceContext));
          this->pRenderTarget = pDeviceContext;
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
          MJ_ERR_HRESULT(
              ::CreateDXGIFactory2(dxgiFlags, __uuidof(pDxgiFactory), reinterpret_cast<void**>(&pDxgiFactory)));
          // MJ_ERR_HRESULT(pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory)));
          // MJ_DEFER(pDxgiFactory->Release());

          // Allocate a descriptor.
          DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
          swapChainDesc.Format                = DXGI_FORMAT_B8G8R8A8_UNORM; // this is the most common swapchain format
          swapChainDesc.Stereo                = false;
          swapChainDesc.SampleDesc.Count      = 1; // don't use multi-sampling
          swapChainDesc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
          swapChainDesc.BufferCount           = 2;                                // use double buffering to enable flip
          swapChainDesc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // all apps must use this SwapEffect
          swapChainDesc.Flags                 = 0;
          swapChainDesc.AlphaMode             = DXGI_ALPHA_MODE_PREMULTIPLIED;

          swapChainDesc.Width  = rect.right - rect.left;
          swapChainDesc.Height = rect.bottom - rect.top;

          // Get the final swap chain for this window from the DXGI factory.

          MJ_ERR_HRESULT(pDxgiFactory->CreateSwapChainForComposition(pDxgiDevice,    //
                                                                     &swapChainDesc, //
                                                                     nullptr,        // allow on all displays
                                                                     &this->pSwapChain));

          // Retrieve the swap chain's back buffer
          IDXGISurface2* pSurface;
          MJ_ERR_HRESULT(this->pSwapChain->GetBuffer(0, // index
                                                     __uuidof(pSurface), reinterpret_cast<void**>(&pSurface)));
          // Create a Direct2D bitmap that points to the swap chain surface
          D2D1_BITMAP_PROPERTIES1 properties = {};
          properties.pixelFormat.alphaMode   = D2D1_ALPHA_MODE_PREMULTIPLIED;
          properties.pixelFormat.format      = DXGI_FORMAT_B8G8R8A8_UNORM;
          properties.bitmapOptions           = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
          ID2D1Bitmap1* pBitmap;
          MJ_ERR_HRESULT(pDeviceContext->CreateBitmapFromDxgiSurface(pSurface, properties, &pBitmap));
          // Point the device context to the bitmap for rendering
          pDeviceContext->SetTarget(pBitmap);

          MJ_ERR_HRESULT(
              ::DCompositionCreateDevice(pDxgiDevice, __uuidof(dcompDevice), reinterpret_cast<void**>(&dcompDevice)));

          // TODO: This is a race
          HWND hWnd = svc::MainWindowHandle();
          IDCompositionTarget* target;
          MJ_ERR_HRESULT(dcompDevice->CreateTargetForHwnd(hWnd,
                                                                       true, // Top most
                                                                       &target));

          IDCompositionVisual* visual;
          MJ_ERR_HRESULT(dcompDevice->CreateVisual(&visual));
          MJ_ERR_HRESULT(visual->SetContent(pSwapChain));
          MJ_ERR_HRESULT(target->SetRoot(visual));
          MJ_ERR_HRESULT(dcompDevice->Commit());
        }
      }
    }
#endif
  }

  virtual void OnDone() override
  {
    this->pMainWindow->OnCreateID2D1RenderTarget(this->pRenderTarget, this->pSwapChain, this->dcompDevice);
  }
};

struct CreateDWriteFactoryTask : public mj::Task
{
  // In
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;

  // Out
  MJ_UNINITIALIZED IDWriteFactory* pDWriteFactory;

  virtual void Execute() override
  {
    ZoneScoped;

    MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                         reinterpret_cast<IUnknown**>(&pDWriteFactory)));
  }

  virtual void OnDone() override
  {
    svc::ProvideDWriteFactory(pDWriteFactory);
    pDWriteFactory->Release();
  }
};

void mj::MainWindow::OnCreateID2D1RenderTarget(ID2D1RenderTarget* pRenderTarget, IDXGISwapChain1* pSwapChain,
                                               IDCompositionDevice* dcompDevice)
{
  this->pRenderTarget = pRenderTarget;
  this->pSwapChain    = pSwapChain;
  this->dcompDevice   = dcompDevice;
  svc::ProvideD2D1RenderTarget(pRenderTarget);
  res::d2d1::Load(pRenderTarget);

  // Make sure we bind to a DC before getting a WM_PAINT message
  this->Resize();
}

#if 0
  // TODO: Put this somewhere else
  {
    ZoneScopedN("GetLogicalDriveStringsW");
    DWORD dw      = ::GetLogicalDriveStringsW(0, nullptr);
    wchar_t* pBuf = pAllocator->New<wchar_t>(dw);
    MJ_DEFER(pAllocator->Free(pBuf));
    dw           = ::GetLogicalDriveStringsW(dw, pBuf);
    wchar_t* ptr = pBuf;
    MJ_UNINITIALIZED StringView str;
    str.Init(nullptr);
    while (ptr < pBuf + dw)
    {
      if (*ptr != 0)
      {
        wchar_t* pBegin = ptr;
        while (*ptr != 0)
        {
          ptr++;
        }
        str.Init(pBegin, ptr - pBegin);
      }
      ptr++;
    }
  }
#endif

void mj::MainWindow::Resize()
{
  ZoneScoped;
  auto hwnd = svc::MainWindowHandle();
  if (pRenderTarget && hwnd)
  {
    MJ_UNINITIALIZED RECT clientArea;
    ::GetClientRect(hwnd, &clientArea);

    if (this->pRootControl)
    {
      this->pRootControl->xParent = 0;
      this->pRootControl->yParent = 0;
      this->pRootControl->width   = static_cast<int16_t>(clientArea.right - clientArea.left);
      this->pRootControl->height  = static_cast<int16_t>(clientArea.bottom - clientArea.top);
      this->pRootControl->OnSize();
    }

    // MJ_ERR_HRESULT(pRenderTarget->BindDC(::GetDC(hwnd), &clientArea));
  }
}

void mj::MainWindow::OnPaint()
{
  ZoneScoped;
  if (this->pRenderTarget)
  {
    {
      ZoneScopedN("BeginDraw");
      this->pRenderTarget->BeginDraw();
    }

    {
      ZoneScopedN("Clear");
      this->pRenderTarget->Clear(D2D1::ColorF(1.0f, 1.0f, 1.0f));
    }

    if (this->pRootControl)
    {
      this->pRootControl->OnPaint();
    }

    {
      ZoneScopedN("EndDraw");
      MJ_ERR_HRESULT(this->pRenderTarget->EndDraw());
    }

    // Make the swap chain available to the composition engine
    MJ_ERR_HRESULT(this->pSwapChain->Present(1,   // sync
                                             0)); // flags
  }
}

void mj::MainWindow::Destroy()
{
  ZoneScoped;

  {
    ZoneScopedN("Controls");
    for (int32_t i = 0; i < MJ_COUNTOF(this->controls); i++)
    {
      Control* pControl = this->controls[i];
      if (pControl)
      {
        pControl->Destroy();
        svc::GeneralPurposeAllocator()->Free(pControl);
        this->controls[i] = nullptr;
      }
    }
  }

  {
    ZoneScopedN("HorizontalLayouts");
    for (int32_t i = 0; i < MJ_COUNTOF(this->pHorizontalLayouts); i++)
    {
      HorizontalLayout* pLayout = this->pHorizontalLayouts[i];
      if (pLayout)
      {
        pLayout->Destroy();
        svc::GeneralPurposeAllocator()->Free(pLayout);
        this->pHorizontalLayouts[i] = nullptr;
      }
    }
  }

  if (this->pRootControl)
  {
    ZoneScopedN("pRootControl->Destroy()");
    this->pRootControl->Destroy();
    // TODO: We should store the allocator used for this allocation, somewhere.
    svc::GeneralPurposeAllocator()->Free(this->pRootControl);
    this->pRootControl = nullptr;
  }

  if (this->pRenderTarget)
  {
    ZoneScopedN("RenderTarget");
    res::d2d1::Destroy();
    svc::ProvideD2D1RenderTarget(nullptr);
    static_cast<void>(pRenderTarget->Release());
    this->pRenderTarget = nullptr;
  }

  svc::Destroy();

  {
    ZoneScopedN("CoUninitialize");
    ::CoUninitialize();
  }
}

namespace mj
{
  static POINTS ScreenPointToClient(HWND hWnd, POINTS ptScreen)
  {
    MJ_UNINITIALIZED POINT clientCoordinates;
    POINTSTOPOINT(clientCoordinates, ptScreen);
    // No fallback if this fails
    static_cast<void>(::ScreenToClient(hWnd, &clientCoordinates));
    MJ_UNINITIALIZED POINTS ptClient;
    ptClient.x = static_cast<SHORT>(clientCoordinates.x);
    ptClient.y = static_cast<SHORT>(clientCoordinates.y);
    return ptClient;
  }
} // namespace mj

/// <summary>
/// Main WindowProc function
/// </summary>
LRESULT CALLBACK mj::MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  mj::MainWindow* pMainWindow = reinterpret_cast<mj::MainWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    ZoneScopedN("WM_NCCREATE");
    // Loaded DLLs: uxtheme, combase, msctf, oleaut32

    // Copy the lpParam from CreateWindowEx to this window's user data
    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow       = reinterpret_cast<mj::MainWindow*>(pcs->lpCreateParams);
    MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));
    svc::ProvideMainWindowHandle(hWnd);
    pMainWindow->Resize();

    // Disable animations, even before presenting the window
    BOOL ani = TRUE;
    ::DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &ani, sizeof(ani));

    return ::DefWindowProcW(hWnd, message, wParam, lParam);
  }
  case WM_SIZE:
    pMainWindow->Resize();
    MJ_ERR_ZERO(::InvalidateRect(hWnd, nullptr, FALSE));
    return 0;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  case WM_PAINT:
  {
    static constexpr const char* pFrameMark = STR(WM_PAINT);
    FrameMarkStart(pFrameMark);
    MJ_UNINITIALIZED PAINTSTRUCT ps;
    static_cast<void>(::BeginPaint(hWnd, &ps));
    pMainWindow->OnPaint();
    static_cast<void>(::EndPaint(hWnd, &ps));
    FrameMarkEnd(pFrameMark);
    return 0;
  }
  case WM_MOUSEMOVE:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (pMainWindow->pRootControl)
    {
      pMainWindow->pRootControl->OnMouseMove(ptClient.x, ptClient.y);
    }
    return 0;
  }
  // Note: "Left" means "Primary" when dealing with mouse buttons.
  // If GetSystemMetrics(SM_SWAPBUTTON) is nonzero, the right mouse button
  // will trigger left mouse button actions, and vice versa.
  case WM_LBUTTONDOWN:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (pMainWindow->pRootControl)
    {
      static_cast<void>(pMainWindow->pRootControl->OnLeftButtonDown(ptClient.x, ptClient.y));
    }
    return 0;
  }
  case WM_LBUTTONUP:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (pMainWindow->pRootControl)
    {
      static_cast<void>(pMainWindow->pRootControl->OnLeftButtonUp(ptClient.x, ptClient.y));
    }
    return 0;
  }
  case WM_LBUTTONDBLCLK:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (pMainWindow->pRootControl)
    {
      pMainWindow->pRootControl->OnDoubleClick(ptClient.x, ptClient.y, static_cast<uint16_t>(wParam));
    }
    return 0;
  }
  case WM_CONTEXTMENU:
  {
    POINTS ptScreen = MAKEPOINTS(lParam);
    POINTS ptClient = mj::ScreenPointToClient(hWnd, ptScreen);
    if (pMainWindow->pRootControl)
    {
      pMainWindow->pRootControl->OnContextMenu(ptClient.x, ptClient.y, ptScreen.x, ptScreen.y);
    }
    return 0;
  }
  case WM_MOUSEWHEEL:
  {
    auto fwKeys     = GET_KEYSTATE_WPARAM(wParam);
    auto zDelta     = GET_WHEEL_DELTA_WPARAM(wParam);
    POINTS ptScreen = MAKEPOINTS(lParam);
    POINTS ptClient = mj::ScreenPointToClient(hWnd, ptScreen);
    if (pMainWindow->pRootControl)
    {
      pMainWindow->pRootControl->OnMouseWheel(ptClient.x, ptClient.y, fwKeys, zDelta);
    }
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

  mj::HeapAllocator generalPurposeAllocator;
  generalPurposeAllocator.Init();
  MJ_UNINITIALIZED mj::AllocatorBase* pAllocator;
  pAllocator = &generalPurposeAllocator;
  svc::ProvideGeneralPurposeAllocator(pAllocator);

  // Initialize thread pool
  MJ_UNINITIALIZED MSG msg;
  MJ_UNINITIALIZED HANDLE iocp;
  MJ_ERR_IF(iocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0), nullptr);
  MJ_DEFER(::CloseHandle(iocp));
  mj::ThreadpoolInit(iocp);
  MJ_DEFER(mj::ThreadpoolDestroy());

  // Start a bunch of tasks
  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateIWICImagingFactoryContext>();
    pTask->pMainWindow = this;
    mj::ThreadpoolSubmitTask(pTask);
  }
  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateID2D1RenderTargetContext>();
    pTask->pMainWindow = this;
    pTask->rect.left   = 0;
    pTask->rect.right  = 1280;
    pTask->rect.top    = 0;
    pTask->rect.bottom = 720;
    mj::ThreadpoolSubmitTask(pTask);
  }
  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateDWriteFactoryTask>();
    pTask->pMainWindow = this;
    mj::ThreadpoolSubmitTask(pTask);
  }

  {
    res::d2d1::Init(pAllocator);
    svc::Init(pAllocator);

    this->pRootControl = pAllocator->New<VerticalLayout>();
    this->pRootControl->Init(pAllocator);

    for (int32_t i = 0; i < MJ_COUNTOF(this->pHorizontalLayouts); i++)
    {
      this->pHorizontalLayouts[i] = pAllocator->New<HorizontalLayout>();
      this->pHorizontalLayouts[i]->Init(pAllocator);

      this->pRootControl->Add(this->pHorizontalLayouts[i]);
    }

    for (int32_t i = 0; i < MJ_COUNTOF(this->controls); i++)
    {
      this->controls[i] = pAllocator->New<DirectoryNavigationPanel>();
      this->controls[i]->Init(pAllocator);

      this->pHorizontalLayouts[i / WIDTH]->Add(this->controls[i]);
    }
  }

  MJ_UNINITIALIZED ATOM cls;
  WNDCLASSEXW wc   = {};
  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.lpfnWndProc   = mj::MainWindow::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = L"Class Name";
  wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.style         = CS_DBLCLKS;
  MJ_ERR_ZERO(cls = ::RegisterClassExW(&wc));
  LPWSTR classAtom = MAKEINTATOM(cls);

  MJ_UNINITIALIZED HWND hWnd;
  {
    ZoneScopedN("CreateWindowExW");
    hWnd = ::CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,        // Optional window styles.
                             classAtom,                        // Window class
                             L"Window Title",                  // Window text
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE, // Window style
                             // WS_POPUP | WS_VISIBLE,                   // Window style
                             CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, // Size and pCurrent
                             nullptr,                                 // Parent window
                             nullptr,                                 // Menu
                             HINST_THISCOMPONENT,                     // Instance handle
                             this);                                   // Additional application data
  }

  // Run the message loop.
  while (true)
  {
    MJ_UNINITIALIZED DWORD waitObject;
    {
      waitObject = ::MsgWaitForMultipleObjects(1, &iocp, FALSE, INFINITE, QS_ALLEVENTS);
    }
    switch (waitObject)
    {
    case WAIT_OBJECT_0: // IOCP
    {
      MJ_UNINITIALIZED DWORD numBytes;
      MJ_UNINITIALIZED mj::Task* pTask;
      MJ_UNINITIALIZED LPOVERLAPPED pOverlapped;

      BOOL ret =
          ::GetQueuedCompletionStatus(iocp, &numBytes, reinterpret_cast<PULONG_PTR>(&pTask), &pOverlapped, INFINITE);
      if (ret == FALSE && ::GetLastError() == ERROR_ABANDONED_WAIT_0)
      {
        return;
      }
      else if (pTask)
      {
        mj::ThreadpoolTaskEnd(pTask);
      }
    }
    break;
    case WAIT_OBJECT_0 + 1: // Window message
    {
      while (::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
      {
        // If the message is translated, the return value is nonzero.
        // If the message is not translated, the return value is zero.
        static_cast<void>(::TranslateMessage(&msg));

        // The return value specifies the value returned by the window procedure.
        // Although its meaning depends on the message being dispatched,
        // the return value generally is ignored.
        static_cast<void>(::DispatchMessageW(&msg));

        if (msg.message == WM_QUIT)
        {
          return;
        }
      }
    }
    break;
    default:
      // TODO: Show error on WAIT_FAILED
      break;
    }
  }
}
