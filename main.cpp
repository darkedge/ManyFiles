#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <dwrite.h>
#include <d2d1.h>
#include <shellscalingapi.h> // MDT_EFFECTIVE_DPI

#include "mj_common.h"
#include "mj_textedit.h"
#include "render_target_resources.h"
#include "resource.h"

static constexpr LPCWSTR pWindowClassName = L"DemoApp";
static constexpr FLOAT s_FontSize         = 10.0f;

static constexpr UINT WINDOW_WIDTH  = 640;
static constexpr UINT WINDOW_HEIGHT = 480;
static constexpr DWORD dwStyle      = WS_OVERLAPPEDWINDOW;

// One logical inch equals 96 pixels. // TODO: This can change!
static constexpr float MJ_96_DPI = 96.0f;
// In typography, the size of type is measured in units called points. One point equals 1/72 of an inch.
static constexpr float MJ_POINT = (1.0f / 72.0f);

// __ImageBase is better than GetCurrentModule()
// Can be cast to a HINSTANCE
#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

static HWND s_Hwnd;

static HCURSOR s_cursorType;

// how much to scale a design that assumes 96-DPI pixels
static float s_DpiScale;

// Direct2D
static ID2D1Factory* s_pD2DFactory;
static ID2D1HwndRenderTarget* s_pRenderTarget;

// DirectWrite
static IDWriteFactory* s_pDWriteFactory;
static IDWriteTextFormat* s_pTextFormat;
static RenderTargetResources s_RenderTargetResources;

static mj::TextEdit s_TextEdit;

#define SAFE_RELEASE(ptr) \
  do \
  { \
    if (ptr) \
    { \
      ptr->Release(); \
      ptr = nullptr; \
    } \
  } while (0)

static HRESULT CreateRenderTargetResources(RenderTargetResources* pRenderTargetResources)
{
  // Create a black brush.
  HRESULT hr =
      s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pRenderTargetResources->pTextBrush);

  // Brush for render target background
  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::WhiteSmoke),
                                                &pRenderTargetResources->pTextEditBackgroundBrush);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CadetBlue),
                                                &pRenderTargetResources->pScrollBarBrush);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkBlue),
                                                &pRenderTargetResources->pScrollBarBackgroundBrush);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::OrangeRed),
                                                &pRenderTargetResources->pScrollBarHighlightBrush);
  }

  return hr;
}

static void DestroyRenderTargetResources(RenderTargetResources* pRenderTargetResources)
{
  SAFE_RELEASE(pRenderTargetResources->pTextBrush);
  SAFE_RELEASE(pRenderTargetResources->pTextEditBackgroundBrush);
  SAFE_RELEASE(pRenderTargetResources->pScrollBarBrush);
  SAFE_RELEASE(pRenderTargetResources->pScrollBarBackgroundBrush);
  SAFE_RELEASE(pRenderTargetResources->pScrollBarHighlightBrush);
}

static void ReleaseResources()
{
  SAFE_RELEASE(s_pD2DFactory);
  SAFE_RELEASE(s_pRenderTarget);
  DestroyRenderTargetResources(&s_RenderTargetResources);
  SAFE_RELEASE(s_pDWriteFactory);
  SAFE_RELEASE(s_pTextFormat);
}

static void OnResize(UINT width, UINT height)
{
  if (s_pRenderTarget)
  {
    D2D1_SIZE_U size;
    size.width  = width;
    size.height = height;

    s_pRenderTarget->Resize(size);
  }
}

static HMONITOR GetPrimaryMonitor()
{
  return MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
}

static HRESULT CreateDeviceResources()
{
  HRESULT hr = S_OK;

  RECT rect;
  GetClientRect(s_Hwnd, &rect);

  D2D1_SIZE_U size = D2D1::SizeU(rect.right - rect.left, rect.bottom - rect.top);

  if (!s_pRenderTarget)
  {
    UINT dpi = GetDpiForWindow(s_Hwnd);
    if (dpi > 0)
    {

      // Create a Direct2D render target.
      MJ_UNINITIALIZED D2D1_RENDER_TARGET_PROPERTIES rtp;
      rtp.type                  = D2D1_RENDER_TARGET_TYPE_DEFAULT;
      rtp.pixelFormat.format    = DXGI_FORMAT_UNKNOWN;
      rtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_UNKNOWN;
      rtp.dpiX                  = (FLOAT)dpi;
      rtp.dpiY                  = (FLOAT)dpi;
      rtp.usage                 = D2D1_RENDER_TARGET_USAGE_NONE;
      rtp.minLevel              = D2D1_FEATURE_LEVEL_DEFAULT;

      MJ_UNINITIALIZED D2D1_HWND_RENDER_TARGET_PROPERTIES hrtp;
      hrtp.hwnd             = s_Hwnd;
      hrtp.pixelSize.width  = 0;
      hrtp.pixelSize.height = 0;
      hrtp.presentOptions   = D2D1_PRESENT_OPTIONS_IMMEDIATELY;

      hr = s_pD2DFactory->CreateHwndRenderTarget(MJ_REF rtp, MJ_REF hrtp, &s_pRenderTarget);
    }

    if (SUCCEEDED(hr))
    {
      hr = CreateRenderTargetResources(&s_RenderTargetResources);
    }
  }

  if (SUCCEEDED(hr))
  {
    // Create a text layout using the text format.
    hr = s_TextEdit.CreateDeviceResources(s_pDWriteFactory, s_pTextFormat, (FLOAT)rect.right, (FLOAT)rect.bottom);
  }

  return hr;
}

static HRESULT DrawD2DContent()
{
  HRESULT hr = CreateDeviceResources();

  if (!(s_pRenderTarget->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
  {
    s_pRenderTarget->BeginDraw();
    s_pRenderTarget->SetTransform(D2D1::IdentityMatrix());
    s_pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::LightPink));

    if (SUCCEEDED(hr))
    {
      s_TextEdit.Draw(s_pRenderTarget, &s_RenderTargetResources);
    }

    if (SUCCEEDED(hr))
    {
      hr = s_pRenderTarget->EndDraw();
    }
  }

  if (FAILED(hr))
  {
    s_pRenderTarget->Release();
    DestroyRenderTargetResources(&s_RenderTargetResources);
  }

  return hr;
}

static void CalculateDpiScale()
{
  UINT dpi   = GetDpiForWindow(s_Hwnd);
  s_DpiScale = (float)dpi / MJ_96_DPI;
}

static FLOAT ConvertPointSizeToDIP(FLOAT points)
{
  return (points * MJ_POINT) * MJ_96_DPI;
}

static HRESULT CreateDeviceIndependentResources()
{
  HRESULT hr;

  // Create Direct2D factory.
  if (s_pD2DFactory)
    s_pD2DFactory->Release();
#ifdef _DEBUG
  D2D1_FACTORY_OPTIONS options;
  options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
  hr                 = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, &s_pD2DFactory);
#else
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &s_pD2DFactory);
#endif // _DEBUG

  // Create a shared DirectWrite factory.
  if (SUCCEEDED(hr))
  {
    if (s_pDWriteFactory)
      s_pDWriteFactory->Release();
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(&s_pDWriteFactory));
  }

  // This sets the default font, weight, stretch, style, and locale.
  if (SUCCEEDED(hr))
  {
    if (s_pTextFormat)
    {
      s_pTextFormat->Release();
    }
    hr = s_pDWriteFactory->CreateTextFormat(
        L"Consolas", // Font family name.
        nullptr,     // Font collection (nullptr sets it to use the system font collection).
        DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ConvertPointSizeToDIP(s_FontSize), L"en-us", &s_pTextFormat);
  }

  if (SUCCEEDED(hr))
  {
    s_pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
  }

  return hr;
}

// iswprint implementation
static bool IsPrintableCharacterWide(wint_t c)
{
  return (c > 31 && c < 127);
}

static mj::ECursor::Enum s_Cursor;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_CAPTURECHANGED:
    s_TextEdit.MouseUp();
    break;
  case WM_MOUSEMOVE:
    s_Cursor = s_TextEdit.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    if (s_TextEdit.GetDragAction().draggable != mj::EDraggable::NONE)
    {
      DrawD2DContent();
    }
    break;
  case WM_LBUTTONDOWN:
  {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    s_TextEdit.MouseDown((SHORT)pt.x, (SHORT)pt.y);
    DrawD2DContent();
    MJ_DISCARD(SetCapture(hwnd));
  }
  break;
  case WM_LBUTTONUP:
    s_TextEdit.MouseUp();
    MJ_DISCARD(ReleaseCapture());
    break;
  case WM_CHAR:
    if (IsPrintableCharacterWide((wint_t)wParam))
    {
      s_TextEdit.WndProc(hwnd, message, wParam, lParam);
      DrawD2DContent();
    }
    break;
  case WM_KEYDOWN:
    switch (wParam)
    {
    case VK_HOME:
    case VK_END:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_DELETE:
    case VK_BACK:
      s_TextEdit.WndProc(hwnd, message, wParam, lParam);
      DrawD2DContent();
      break;
    }
    break;
  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT)
    {
      switch (s_Cursor)
      {
      case mj::ECursor::IBEAM:
        SetCursor(s_cursorType);
        return TRUE;
      default:
        break;
      }
    }
    break;
  case WM_SIZE:
  {
    UINT width  = LOWORD(lParam);
    UINT height = HIWORD(lParam);
    OnResize(width, height);
  }
  break;
  case WM_DPICHANGED:
  {
    CalculateDpiScale();
    CreateDeviceIndependentResources();

    DestroyRenderTargetResources(&s_RenderTargetResources);
    SAFE_RELEASE(s_pRenderTarget);
    CreateDeviceResources();
  }
  break;
  case WM_PAINT:
  case WM_DISPLAYCHANGE:
    ValidateRect(hwnd, nullptr);
    DrawD2DContent();
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

void __stdcall WinMainCRTStartup()
{
  HRESULT hr = s_TextEdit.Init(20.0f, 10.0f, WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);

  if (SUCCEEDED(hr))
  {
    // Register window class.
    {
      MJ_UNINITIALIZED WNDCLASSEX wcex;
      wcex.cbSize        = sizeof(WNDCLASSEX);
      wcex.style         = CS_HREDRAW | CS_VREDRAW;
      wcex.lpfnWndProc   = WndProc;
      wcex.cbClsExtra    = 0;
      wcex.cbWndExtra    = sizeof(LONG_PTR);
      wcex.hInstance     = HINST_THISCOMPONENT;
      wcex.hbrBackground = nullptr;
      wcex.lpszMenuName  = MAKEINTRESOURCEW(IDR_MENU1);
      wcex.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
      wcex.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
      wcex.lpszClassName = pWindowClassName;
      wcex.hIconSm       = LoadIconW(nullptr, IDI_APPLICATION);

      hr = RegisterClassExW(&wcex) ? S_OK : E_FAIL;
    }

    s_cursorType = LoadCursorW(nullptr, IDC_IBEAM);
  }

  // We currently assume that the application will always be created on the primary monitor.
  MJ_UNINITIALIZED UINT dpiX;
  MJ_UNINITIALIZED UINT dpiY;
  hr = GetDpiForMonitor(GetPrimaryMonitor(), MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
  if (SUCCEEDED(hr))
  {
    s_DpiScale = (float)dpiX / MJ_96_DPI;
  }

  if (SUCCEEDED(hr))
  {
    hr = CreateDeviceIndependentResources();
  }

  if (SUCCEEDED(hr))
  {
    // Get window rectangle
    RECT windowRect       = { 0, 0, (LONG)(WINDOW_WIDTH * s_DpiScale), (LONG)(WINDOW_HEIGHT * s_DpiScale) };
    const bool hasMenu    = false;
    const DWORD dwExStyle = 0;
    AdjustWindowRectExForDpi(&windowRect, dwStyle, hasMenu, dwExStyle, dpiX);
    const LONG windowWidth  = windowRect.right - windowRect.left;
    const LONG windowHeight = windowRect.bottom - windowRect.top;

    // Create window.
    s_Hwnd = CreateWindowExW(0, pWindowClassName, L"hardcalc", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                             static_cast<int>(windowWidth), static_cast<int>(windowHeight), nullptr, nullptr,
                             HINST_THISCOMPONENT, nullptr);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_Hwnd ? S_OK : E_FAIL;
  }

  // Draw initial contents to prevent a blank screen flash
  if (SUCCEEDED(hr))
  {
    hr = DrawD2DContent();
  }

  if (SUCCEEDED(hr))
  {
    ShowWindow(s_Hwnd, SW_SHOWNORMAL);
    UpdateWindow(s_Hwnd);
  }

  if (SUCCEEDED(hr))
  {
    MSG msg;

    // Event loop
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  s_TextEdit.Destroy();

  ReleaseResources();

  ExitProcess(0);
}
