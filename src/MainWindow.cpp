#include "MainWindow.h"
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "DirectoryNavigationPanel.h"
#include <dwrite.h>
#include <wincodec.h> // WIC
#include "../3rdparty/tracy/Tracy.hpp"
#include "Threadpool.h"
#include "ResourcesD2D1.h"
#include "ResourcesWin32.h"

#include "HorizontalLayout.h"
#include "VerticalLayout.h"

#include <dwmapi.h>
#include <d3d11.h>
#include <dcomp.h>

#include <vssym32.h>

#define WM_MJTASKFINISH (WM_USER + 1)

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
  MJ_UNINITIALIZED ID2D1DeviceContext* pRenderTarget;
  MJ_UNINITIALIZED IDCompositionDesktopDevice* pDCompDevice;

  virtual void Execute() override
  {
    ZoneScoped;
#ifdef _DEBUG
    UINT creationFlags              = D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
    D2D1_DEBUG_LEVEL d2d1DebugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
    UINT creationFlags              = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D2D1_DEBUG_LEVEL d2d1DebugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif
    MJ_UNINITIALIZED ID3D11Device* pD3d11Device;
    {
      ZoneScopedN("D3D11CreateDevice");
      MJ_ERR_HRESULT(::D3D11CreateDevice(nullptr,                  // specify null to use the default adapter
                                         D3D_DRIVER_TYPE_HARDWARE, //
                                         0,                        //
                                         creationFlags,     // optionally set debug and Direct2D compatibility flags
                                         nullptr,           // list of feature levels this app can support
                                         0,                 // number of possible feature levels
                                         D3D11_SDK_VERSION, //
                                         &pD3d11Device,     // returns the Direct3D device created
                                         nullptr,           // returns feature level of device created
                                         nullptr            // No ID3D11DeviceContext will be returned.
                                         ));
    }
    MJ_DEFER(pD3d11Device->Release());

    // Obtain the underlying DXGI device of the Direct3D11 device.
    {
      MJ_UNINITIALIZED IDXGIDevice1* pDxgiDevice;
      MJ_ERR_HRESULT(pD3d11Device->QueryInterface<IDXGIDevice1>(&pDxgiDevice));
      MJ_DEFER(pDxgiDevice->Release());

      // Ensure that DXGI doesn't queue more than one frame at a time.
      // MJ_ERR_HRESULT(pDxgiDevice->SetMaximumFrameLatency(1));

      // Obtain the Direct2D device for 2-D rendering.
      MJ_UNINITIALIZED ID2D1Device* pD2d1Device;
      {
        MJ_ERR_HRESULT(::D2D1CreateDevice(pDxgiDevice,
                                          D2D1::CreationProperties(D2D1_THREADING_MODE_MULTI_THREADED, d2d1DebugLevel,
                                                                   D2D1_DEVICE_CONTEXT_OPTIONS_NONE),
                                          &pD2d1Device));

        // Get Direct2D device's corresponding device context object.
        MJ_ERR_HRESULT(pD2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &this->pRenderTarget));
      }
      MJ_DEFER(pD2d1Device->Release());

      {
        ZoneScopedN("DCompositionCreateDevice");
        MJ_ERR_HRESULT(::DCompositionCreateDevice2(pD2d1Device, IID_PPV_ARGS(&this->pDCompDevice)));
      }
    }
  }

  virtual void OnDone() override
  {
    this->pMainWindow->OnCreateID2D1RenderTarget(this->pDCompDevice, this->pRenderTarget);
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

void mj::MainWindow::OnCreateID2D1RenderTarget(IDCompositionDesktopDevice* dcompDevice,
                                               ID2D1RenderTarget* pRenderTarget)
{

  this->dcompDevice = dcompDevice;
  svc::ProvideD2D1RenderTarget(pRenderTarget);
  res::d2d1::Load(pRenderTarget);

  MJ_UNINITIALIZED IDCompositionVisual2* pVisual;
  MJ_ERR_HRESULT(this->dcompDevice->CreateVisual(&pVisual));
  MJ_DEFER(pVisual->Release());

  MJ_ERR_HRESULT(this->dcompDevice->CreateTargetForHwnd(svc::MainWindowHandle(), true, &this->pTarget));
  MJ_ERR_HRESULT(this->pTarget->SetRoot(pVisual));

  MJ_ERR_HRESULT(this->dcompDevice->CreateVirtualSurface(0, 0, DXGI_FORMAT_B8G8R8A8_UNORM,
                                                         DXGI_ALPHA_MODE_PREMULTIPLIED, &this->pSurface));
  MJ_ERR_HRESULT(pVisual->SetContent(this->pSurface));

  // If the window was previously visible, the return value is nonzero.
  // If the window was previously hidden, the return value is zero.
  static_cast<void>(::ShowWindow(svc::MainWindowHandle(), SW_SHOW));
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
  HWND hWnd = svc::MainWindowHandle();
  // TODO: Check if IsIconic() can be removed after adding a WM_NCCALCSIZE message handler
  if (hWnd && !::IsIconic(hWnd))
  {
    MJ_UNINITIALIZED RECT clientArea;
    MJ_ERR_IF(::GetClientRect(hWnd, &clientArea), 0);

    UINT width  = clientArea.right - clientArea.left;
    UINT height = clientArea.bottom - clientArea.top;

    if (this->pSurface)
    {
      MJ_ERR_HRESULT(this->pSurface->Resize(width, height));
    }

    if (this->pRootControl)
    {
      this->pRootControl->xParent = 0;
      this->pRootControl->yParent = this->captionHeight;
      this->pRootControl->width   = static_cast<int16_t>(width);
      this->pRootControl->height  = static_cast<int16_t>(height);
      this->pRootControl->OnSize();
    }
  }
  MJ_ERR_ZERO(::InvalidateRect(hWnd, nullptr, FALSE));
}

void mj::MainWindow::OnPaint()
{
  ZoneScoped;

  if (this->pSurface)
  {
    MJ_UNINITIALIZED ID2D1DeviceContext* pContext;
    MJ_UNINITIALIZED POINT offset;
    MJ_ERR_HRESULT(this->pSurface->BeginDraw(nullptr, IID_PPV_ARGS(&pContext), &offset));
    MJ_DEFER(pContext->Release());

    pContext->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(offset.x, offset.y)));
    pContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

    if (this->pRootControl)
    {
      this->pRootControl->OnPaint(pContext);
    }

    {
      ZoneScopedN("EndDraw");
      MJ_ERR_HRESULT(this->pSurface->EndDraw());
      MJ_ERR_HRESULT(this->dcompDevice->Commit());
    }
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

  res::d2d1::Destroy();

  ID2D1RenderTarget* pRenderTarget = svc::D2D1RenderTarget();
  if (pRenderTarget)
  {
    ZoneScopedN("RenderTarget");
    svc::ProvideD2D1RenderTarget(nullptr);
    static_cast<void>(pRenderTarget->Release());
    pRenderTarget = nullptr;
  }

  // Note: We have to release these objects in the right order,
  // otherwise the Direct2D debug layer will report memory leaks.
  // Surface, target, device.
  MJ_SAFE_RELEASE(this->pSurface);
  MJ_SAFE_RELEASE(this->pTarget);
  MJ_SAFE_RELEASE(this->dcompDevice);

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

static int compute_standard_caption_height_for_window(HWND window_handle)
{
  ZoneScoped;
  SIZE caption_size{};
  int accounting_for_borders = 2;
  HTHEME theme               = ::OpenThemeData(window_handle, L"WINDOW");
  UINT dpi                   = ::GetDpiForWindow(window_handle);
  ::GetThemePartSize(theme, nullptr, WP_CAPTION, CS_ACTIVE, nullptr, TS_TRUE, &caption_size);
  ::CloseThemeData(theme);

  auto height = static_cast<float>(caption_size.cy * dpi) / 96.0f;
  return static_cast<int>(height) + accounting_for_borders;
}

static LRESULT compute_sector_of_window(HWND window_handle, WPARAM, LPARAM lParam, LONG caption_height)
{
  // Acquire the window rect
  MJ_UNINITIALIZED RECT rect;
  MJ_ERR_ZERO(::GetWindowRect(window_handle, &rect));

  LONG offset = 10;
  POINTS p    = MAKEPOINTS(lParam);

  if (p.y < rect.top + offset && p.x < rect.left + offset)
  {
    return HTTOPLEFT;
  }
  if (p.y < rect.top + offset && p.x > rect.right - offset)
  {
    return HTTOPRIGHT;
  }
  if (p.y > rect.bottom - offset && p.x > rect.right - offset)
  {
    return HTBOTTOMRIGHT;
  }
  if (p.y > rect.bottom - offset && p.x < rect.left + offset)
  {
    return HTBOTTOMLEFT;
  }

  if (p.x > rect.left && p.x < rect.right)
  {
    if (p.y < rect.top + offset)
    {
      return HTTOP;
    }
    else if (p.y > rect.bottom - offset)
    {
      return HTBOTTOM;
    }
  }
  if (p.y > rect.top && p.y < rect.bottom)
  {
    if (p.x < rect.left + offset)
    {
      return HTLEFT;
    }
    else if (p.x > rect.right - offset)
    {
      return HTRIGHT;
    }
  }

  if (p.x > rect.left && p.x < rect.right)
  {
    if (p.y < rect.top + caption_height)
    {
      return HTCAPTION;
    }
  }

  return HTNOWHERE;
}

/// <summary>
/// Main WindowProc function
/// </summary>
LRESULT CALLBACK mj::MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  {
    MJ_UNINITIALIZED LRESULT result;
    if (::DwmDefWindowProc(hWnd, message, wParam, lParam, &result))
    {
      return result;
    }
  }

  mj::MainWindow* pMainWindow = reinterpret_cast<mj::MainWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_CREATE:
  {
    ZoneScopedN("WM_CREATE");
    // Loaded DLLs: uxtheme, combase, msctf, oleaut32

    // Copy the lpParam from CreateWindowEx to this window's user data
    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow       = reinterpret_cast<mj::MainWindow*>(pcs->lpCreateParams);
    MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));
    svc::ProvideMainWindowHandle(hWnd);

    // Disable animations, even before presenting the window
    // TODO: It is better to leave this as a configuration option.
    // BOOL ani = TRUE;
    // ::DwmSetWindowAttribute(hWnd, DWMWA_TRANSITIONS_FORCEDISABLED, &ani, sizeof(ani));

    int offset_for_tabs = 10; // Arbitrary
    int caption_height  = compute_standard_caption_height_for_window(hWnd);
    pMainWindow->SetCaptionHeight(caption_height + offset_for_tabs);

    MARGINS _margins = { 0, 0, pMainWindow->GetCaptionHeight(), 0 };
    MJ_ERR_HRESULT(::DwmExtendFrameIntoClientArea(hWnd, &_margins));

    // Make sure a WM_NCCALCSIZE message is fired that resizes the frame (wParam = true).
    // This cannot be done inside WM_NCCREATE (probably makes sense)
    MJ_ERR_ZERO(::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE));

    return 0;
  }
  case WM_NCCALCSIZE:
    // Remove the non-client area.
    if (wParam)
    {
      NCCALCSIZE_PARAMS* pParams = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

      // Note: rgrc is an array of three RECTs, but we only need the first one.
      RECT* pRect = pParams->rgrc;
      int frameX  = ::GetSystemMetrics(SM_CXFRAME);
      int border  = ::GetSystemMetrics(SM_CXPADDEDBORDER);
      pRect->right -= frameX + border;
      pRect->left += frameX + border;
      pRect->bottom -= ::GetSystemMetrics(SM_CYFRAME) + border;
    }
    return 0;
  case WM_NCHITTEST:
  {
    LRESULT result = compute_sector_of_window(hWnd, wParam, lParam, pMainWindow->GetCaptionHeight());
    if (result != HTNOWHERE)
    {
      return result;
    }
  }
  break; // Let DefWindowProcW handle WM_NCHITTEST
  case WM_SIZE:
    pMainWindow->Resize();
    return 0;
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;
  case WM_PAINT:
  {
    static constexpr const char* pFrameMark = STR(WM_PAINT);
    // We don't present at regular intervals (like a video game)
    // so we use FrameMarkStart/FrameMarkEnd to mark our rendering.
    FrameMarkStart(pFrameMark);
    pMainWindow->OnPaint();
    FrameMarkEnd(pFrameMark);

    // Because we're not calling BeginPaint, we need to manually validate the client region,
    // otherwise the system continues to generate WM_PAINT messages
    // until the current update region is validated.
    static_cast<void>(::ValidateRect(hWnd, nullptr));
    return 0;
  }
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      return TRUE;
    }
    break;
  case WM_MOUSEMOVE:
  {
    POINTS ptClient   = MAKEPOINTS(lParam);
    Control* pControl = pMainWindow->pRootControl;

    MouseMoveEvent mouseMoveEvent; // Initialized
    mouseMoveEvent.x = ptClient.x;
    mouseMoveEvent.y = ptClient.y;

    if (pControl && pControl->TranslateClientPoint(&mouseMoveEvent.x, &mouseMoveEvent.y))
    {
      pControl->OnMouseMove(&mouseMoveEvent);
      res::win32::SetCursor(mouseMoveEvent.cursor);
    }
    return 0;
  }
  // Note: "Left" means "Primary" when dealing with mouse buttons.
  // If GetSystemMetrics(SM_SWAPBUTTON) is nonzero, the right mouse button
  // will fire WM_LBUTTONDOWN, and vice versa.
  case WM_LBUTTONDOWN:
  {
    POINTS ptClient   = MAKEPOINTS(lParam);
    Control* pControl = pMainWindow->pRootControl;
    if (pControl && pControl->TranslateClientPoint(&ptClient.x, &ptClient.y))
    {
      static_cast<void>(pControl->OnLeftButtonDown(ptClient.x, ptClient.y));
    }
    // The return value is a handle to the window that had previously captured the mouse.
    // If there is no such window, the return value is NULL.
    static_cast<void>(::SetCapture(hWnd));
    return 0;
  }
  case WM_LBUTTONUP:
  {
    POINTS ptClient   = MAKEPOINTS(lParam);
    Control* pControl = pMainWindow->pRootControl;
    if (pControl && pControl->TranslateClientPoint(&ptClient.x, &ptClient.y))
    {
      static_cast<void>(pControl->OnLeftButtonUp(ptClient.x, ptClient.y));
    }
    MJ_ERR_ZERO(::ReleaseCapture());
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
  mj::ThreadpoolInit(::GetCurrentThreadId(), WM_MJTASKFINISH);
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

  res::win32::Init();
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

  MJ_UNINITIALIZED ATOM cls;
  WNDCLASSEXW wc   = {};
  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.lpfnWndProc   = mj::MainWindow::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = L"ManyFiles";
  wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
  wc.style         = CS_DBLCLKS;
  MJ_ERR_ZERO(cls = ::RegisterClassExW(&wc));
  LPWSTR classAtom = MAKEINTATOM(cls);

  MJ_UNINITIALIZED HWND hWnd;
  {
    ZoneScopedN("CreateWindowExW");
    hWnd = ::CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP,               // Optional window styles.
                             classAtom,                               // Window class
                             L"ManyFiles",                            // Window text
                             WS_OVERLAPPEDWINDOW,                     // Window style
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

    if (msg.message == WM_MJTASKFINISH)
    {
      // Threadpool notifies the main thread using PostThreadMessage.
      // These messages are not associated with a window, so they must be
      // handled here, instead of in the WindowProc.
      mj::ThreadpoolTaskEnd(reinterpret_cast<mj::Task*>(msg.wParam));
    }
  }
  this->SaveLayoutToFile();
}

void mj::MainWindow::SaveLayoutToFile()
{
  // Skip window x/y width/height for now.

  MJ_UNINITIALIZED HANDLE file;
  MJ_ERR_IF(file = ::CreateFileW(L"window_layout.txt", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                 nullptr),
            INVALID_HANDLE_VALUE);

  ArrayList<wchar_t> al;
  al.Init(svc::GeneralPurposeAllocator());
  MJ_DEFER(al.Destroy());
  StringBuilder sb;
  sb.SetArrayList(&al);

  this->pRootControl->SaveToString(sb, 0);

  StringView sv = sb.ToStringOpen();
  MJ_UNINITIALIZED DWORD bytesWritten;

  // Write the UTF-16 LE BOM here, until we know what's best to do here.
  uint16_t bom = 0xFEFF;
  MJ_ERR_ZERO(::WriteFile(file, &bom, sizeof(bom), &bytesWritten, nullptr));
  MJ_ERR_ZERO(::WriteFile(file, sv.ptr, sv.len * sizeof(wchar_t), &bytesWritten, nullptr));
}
