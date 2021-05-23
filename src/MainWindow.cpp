#include "pch.h"
#include "MainWindow.h"
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "DirectoryNavigationPanel.h"
#include "Threadpool.h"
#include "ResourcesD2D1.h"
#include "ResourcesWin32.h"
#include "WindowLayout.h"
#include "D2D1NullRenderTarget.h"

#include "HorizontalLayout.h"
#include "VerticalLayout.h"
#include "InvalidateRect.h"

#define WM_MJTASKFINISH (WM_USER + 1)

static D2D1NullRenderTarget s_D2D1NullRenderTarget;

struct EInvalidateRectThreadEvent
{
  enum Enum
  {
    InvalidateRect,
    Destroy,
    COUNT
  };
};

static HANDLE s_InvalidateRectThreadEvents[EInvalidateRectThreadEvent::COUNT];

static bool TranslateClientPoint(const mj::Rect& panel, int16_t* pX, int16_t* pY)
{
  int16_t x = *pX;
  int16_t y = *pY;
  return x >= panel.x && x <= panel.x + panel.width && y >= panel.y && y <= panel.y + panel.height;
}

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

    this->panel.Resize(0, 0, static_cast<int16_t>(width), static_cast<int16_t>(height));
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

    pContext->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    pContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    // This line needs to be kept, otherwise painting will jump after receiving certain mouse move events.
    // We offset it with another half pixel because Direct2D coordinates are based on the edges of pixels,
    // instead of the center. As we're doing aliased drawing with integer coordinates, this should prevent
    // any drawing from running off screen.
    pContext->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(offset.x + 0.5f, offset.y + 0.5f)));

    // Something to do for every control
    {
      // pContext->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(this->panel.x, -this->captionHeight +
      // this->panel.y)));

      // D2D1_RECT_F rect = D2D1::RectF(this->panel.x, this->panel.y, this->panel.width, this->panel.height);
      // pContext->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
      // MJ_DEFER(pContext->PopAxisAlignedClip
      auto rootRect = PushRect(pContext, this->panel.rect);
      MJ_DEFER(rootRect.Pop(pContext));
      this->panel.Paint(pContext);

#if 0 // Getting the theme font for captions (instead of hard-coding "Segoe UI")
      HTHEME hTheme = ::OpenThemeData(nullptr, L"WINDOW");
      if (hTheme)
      {
        // Select a font.
        LOGFONT lgFont;
        HFONT hFontOld = nullptr;
        if (SUCCEEDED(::GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &lgFont)))
        {
          HFONT hFont = ::CreateFontIndirect(&lgFont);
        }
        ::CloseThemeData(hTheme);
      }
#endif
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
    ZoneScopedN("pRootControl->Destroy()");
    this->panel.Destroy();
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

void mj::MainWindow::ExtendFrame(HWND hWnd)
{
  ZoneScoped;

  HTHEME theme = ::OpenThemeData(hWnd, L"WINDOW");
  MJ_DEFER(MJ_ERR_HRESULT(::CloseThemeData(theme)));

  SIZE size = {};
  MJ_ERR_HRESULT(::GetThemePartSize(theme, nullptr, WP_CAPTION, CS_ACTIVE, nullptr, TS_TRUE, &size));

  this->margins.cyTopHeight = size.cy * ::GetDpiForWindow(hWnd) / 96 + 2; // 1 pixel border on top and bottom
  MJ_ERR_HRESULT(::DwmExtendFrameIntoClientArea(hWnd, &this->margins));
}

/// <summary>
/// https://docs.microsoft.com/en-us/windows/win32/dwm/customframe
/// </summary>
LRESULT mj::MainWindow::DetermineNonClientWindowArea(HWND hWnd, WPARAM, LPARAM lParam, LONG captionHeight)
{
  POINTS mouse = MAKEPOINTS(lParam);

  // Get the point coordinates for the hit test.

  // Get the window rectangle.
  MJ_UNINITIALIZED RECT window;
  MJ_ERR_ZERO(::GetWindowRect(hWnd, &window));

  // Get the frame rectangle, adjusted for the style without a caption.
  RECT rcFrame = { 0 };
  MJ_ERR_ZERO(::AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL));

  // Determine if the hit test is for resizing. Default middle (1,1).
  USHORT uRow          = 1;
  USHORT uCol          = 1;
  bool fOnResizeBorder = false;

  // Determine if the point is at the top or bottom of the window.
  if (mouse.y >= window.top && mouse.y < window.top + this->margins.cyTopHeight)
  {
    fOnResizeBorder = (mouse.y < (window.top - rcFrame.top));
    uRow            = 0;
  }
  else if (mouse.y < window.bottom && mouse.y >= window.bottom - this->margins.cyBottomHeight)
  {
    uRow = 2;
  }

  // Determine if the point is at the left or right of the window.
  if (mouse.x >= window.left && mouse.x < window.left + this->margins.cxLeftWidth)
  {
    uCol = 0; // left side
  }
  else if (mouse.x < window.right && mouse.x >= window.right - this->margins.cxRightWidth)
  {
    uCol = 2; // right side
  }

  // Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
  LRESULT hitTests[3][3] = {
    { HTTOPLEFT, fOnResizeBorder ? HTTOP : HTCAPTION, HTTOPRIGHT },
    { HTLEFT, HTNOWHERE, HTRIGHT },
    { HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT },
  };

  return hitTests[uRow][uCol];
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

  auto OnMouseMove = [&](POINTS ptClient) {
    MouseMoveEvent mouseMoveEvent; // Initialized
    mouseMoveEvent.x = ptClient.x;
    mouseMoveEvent.y = ptClient.y;

    bool inside       = TranslateClientPoint(pMainWindow->panel.rect, &mouseMoveEvent.x, &mouseMoveEvent.y);
    bool updateCursor = true;

    if (::GetCapture() == hWnd)
    {
      // pMainWindow->panel.MoveResizeControl(mouseMoveEvent.x, mouseMoveEvent.y);
      updateCursor = false;
    }
    else if (inside)
    {
      pMainWindow->panel.OnMouseMove(&mouseMoveEvent);
      if (updateCursor)
      {
        res::win32::SetCursor(mouseMoveEvent.cursor);
      }
    }
  };

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

    pMainWindow->ExtendFrame(hWnd);

    // Make sure a WM_NCCALCSIZE message is fired that resizes the frame (wParam = true).
    // This cannot be done inside WM_NCCREATE (probably makes sense)
    MJ_ERR_ZERO(::SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE));

    return 0;
  }
  case WM_NCCALCSIZE:
    if (wParam == TRUE)
    {
      return 0;
    }
    break;
  case WM_NCHITTEST:
  {
    LRESULT result = pMainWindow->DetermineNonClientWindowArea(hWnd, wParam, lParam, pMainWindow->margins.cyTopHeight);
    if (result != HTNOWHERE)
    {
      return result;
    }
    break; // Else, let DefWindowProcW handle WM_NCHITTEST
  }
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
    OnMouseMove(MAKEPOINTS(lParam));
    return 0;
  }
  // Note: "Left" means "Primary" when dealing with mouse buttons.
  // If GetSystemMetrics(SM_SWAPBUTTON) is nonzero, the right mouse button
  // will fire WM_LBUTTONDOWN, and vice versa.
  case WM_LBUTTONDOWN:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (TranslateClientPoint(pMainWindow->panel.rect, &ptClient.x, &ptClient.y))
    {
      // static_cast<void>(pMainWindow->panel.OnLeftButtonDown(ptClient.x, ptClient.y));
    }
    // Continue receiving WM_MOUSEMOVE messages while the mouse is outside the window.
    // The return value is a handle to the window that had previously captured the mouse.
    // If there is no such window, the return value is NULL.
    static_cast<void>(::SetCapture(hWnd));
    return 0;
  }
  case WM_LBUTTONUP:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    if (TranslateClientPoint(pMainWindow->panel.rect, &ptClient.x, &ptClient.y))
    {
      // static_cast<void>(pMainWindow->panel.OnLeftButtonUp(ptClient.x, ptClient.y));
    }
    MJ_ERR_ZERO(::ReleaseCapture());
    return 0;
  }
  case WM_XBUTTONUP:
  {
    if (HIWORD(wParam) == XBUTTON1) // Mouse4 - Back
    {
      pMainWindow->panel.OnBackButton();
    }
    return 0;
  }
  case WM_LBUTTONDBLCLK:
  {
    POINTS ptClient = MAKEPOINTS(lParam);
    static_cast<void>(TranslateClientPoint(pMainWindow->panel.rect, &ptClient.x, &ptClient.y));
    pMainWindow->panel.OnDoubleClick(ptClient.x, ptClient.y, static_cast<uint16_t>(wParam));
    return 0;
  }
  case WM_CONTEXTMENU:
  {
    POINTS ptScreen = MAKEPOINTS(lParam);
    POINTS ptClient = mj::ScreenPointToClient(hWnd, ptScreen);
    static_cast<void>(TranslateClientPoint(pMainWindow->panel.rect, &ptClient.x, &ptClient.y));
    pMainWindow->panel.OnContextMenu(ptClient.x, ptClient.y, ptScreen.x, ptScreen.y);
    return 0;
  }
  case WM_MOUSEWHEEL:
  {
    auto fwKeys     = GET_KEYSTATE_WPARAM(wParam);
    auto zDelta     = GET_WHEEL_DELTA_WPARAM(wParam);
    POINTS ptScreen = MAKEPOINTS(lParam);
    POINTS ptClient = mj::ScreenPointToClient(hWnd, ptScreen);
    pMainWindow->panel.OnMouseWheel(ptClient.x, ptClient.y, fwKeys, zDelta);
    OnMouseMove(ptClient);
    return 0;
  }
  case WM_KEYDOWN:
    switch (wParam)
    {
    case VK_UP:
      pMainWindow->panel.MoveSelectionUp();
      break;
    case VK_DOWN:
      pMainWindow->panel.MoveSelectionDown();
      break;
    case VK_BACK: // Backspace
      pMainWindow->panel.OnBackButton();
      break;
    default:
      break;
    }
    break;
  default:
    break;
  }

  return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

/// <summary>
/// Currently, we only use this thread to signal InvalidateRect.
/// It could also be used for other purposes.
/// </summary>
static DWORD WINAPI InvalidateRectMain(LPVOID lpThreadParameter)
{
  static_cast<void>(lpThreadParameter);

  while (true)
  {
    switch (::WaitForMultipleObjects(MJ_COUNTOF(s_InvalidateRectThreadEvents), s_InvalidateRectThreadEvents, FALSE,
                                     INFINITE))
    {
    case EInvalidateRectThreadEvent::InvalidateRect:
      MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
      break;
    case EInvalidateRectThreadEvent::Destroy:
      return 0;
    default:
      MJ_EXIT_NULL(0);
      break;
    }
  }

  return 0;
}

/// <summary>
/// This function should not be called from the main thread,
/// unless an immediate repaint is required (should not be the case most of the time).
/// </summary>
void mj::InvalidateRect()
{
  ::SetEvent(s_InvalidateRectThreadEvents[EInvalidateRectThreadEvent::InvalidateRect]);
}

void mj::MainWindow::Run()
{
  MJ_DEFER(this->Destroy());

  s_InvalidateRectThreadEvents[EInvalidateRectThreadEvent::InvalidateRect] =
      ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
  s_InvalidateRectThreadEvents[EInvalidateRectThreadEvent::Destroy] = ::CreateEventW(nullptr, TRUE, FALSE, nullptr);

  // Set this null object because the actual object will not be available for some time
  svc::ProvideD2D1RenderTarget(&s_D2D1NullRenderTarget);

  mj::HeapAllocator generalPurposeAllocator;
  svc::ProvideGeneralPurposeAllocator(&generalPurposeAllocator);
  mj::AllocatorBase* pAllocator = svc::GeneralPurposeAllocator();

  // Initialize thread pool
  mj::ThreadpoolInit(::GetCurrentThreadId(), WM_MJTASKFINISH);
  MJ_DEFER(mj::ThreadpoolDestroy());
  HANDLE pInvalidateRectThread = ::CreateThread(nullptr, 0, InvalidateRectMain, nullptr, 0, nullptr);

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

  // LoadWindowLayout(pAllocator)
  this->panel.Init(pAllocator);
  // MJ_DEFER(SaveWindowLayout(this->pRootControl));

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
                             L"",                                     // Window text
                             WS_THICKFRAME,                           // Window style
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

  ::SetEvent(s_InvalidateRectThreadEvents[EInvalidateRectThreadEvent::Destroy]);
  ::WaitForSingleObject(pInvalidateRectThread, INFINITE);
  ::CloseHandle(pInvalidateRectThread);
  for (auto& handle : s_InvalidateRectThreadEvents)
  {
    ::CloseHandle(handle);
    handle = nullptr;
  }
}
