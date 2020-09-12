#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shobjidl.h>
#include <dwrite.h>
#include <d2d1.h>
#include <shellscalingapi.h> // MDT_EFFECTIVE_DPI

#include "mj_win32.h"
#include "mj_common.h"
#include "mj_textedit.h"
#include "render_target_resources.h"
#include "resource.h"

class Main
{
public:
  static constexpr LPCWSTR pWindowClassName = L"DemoApp";
  static constexpr FLOAT s_FontSize         = 10.0f;

  static constexpr UINT WINDOW_WIDTH  = 1024;
  static constexpr UINT WINDOW_HEIGHT = 768;
  static constexpr DWORD dwStyle      = WS_OVERLAPPEDWINDOW;

  // One logical inch equals 96 pixels. // TODO: This can change!
  static constexpr float MJ_96_DPI = 96.0f;
  // In typography, the size of type is measured in units called points. One point equals 1/72 of an inch.
  static constexpr float MJ_POINT = (1.0f / 72.0f);

  static HRESULT RegisterWindowClass();

  void Start();
  void WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

private:
  HRESULT CreateRenderTargetResources();
  void OnResize(UINT width, UINT height);
  HMONITOR GetPrimaryMonitor();
  void SetWordWrap(bool wordWrap);
  HRESULT CreateDeviceResources();
  HRESULT DrawD2DContent();
  void CalculateDpiScale();
  FLOAT ConvertPointSizeToDIP(FLOAT points);
  HRESULT CreateDeviceIndependentResources();
  HRESULT BasicFileOpen();

  HWND s_Hwnd;

  // how much to scale a design that assumes 96-DPI pixels
  float s_DpiScale;

  // Direct2D
  mj::ComPtr<ID2D1Factory> s_pD2DFactory;
  mj::ComPtr<ID2D1HwndRenderTarget> s_pRenderTarget;

  // DirectWrite
  mj::ComPtr<IDWriteFactory> s_pDWriteFactory;
  mj::ComPtr<IDWriteTextFormat> s_pTextFormat;
  RenderTargetResources s_RenderTargetResources;

  mj::TextEdit s_TextEdit;

  bool s_WordWrap = false;
};

HRESULT Main::CreateRenderTargetResources()
{
  // Create a black brush.
  HRESULT hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                                      s_RenderTargetResources.pTextBrush.ReleaseAndGetAddressOf());

  // Brush for render target background
  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::WhiteSmoke),
        s_RenderTargetResources.pTextEditBackgroundBrush.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::CadetBlue),
                                                s_RenderTargetResources.pScrollBarBrush.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black),
                                                s_RenderTargetResources.pCaretBrush.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::DarkBlue),
        s_RenderTargetResources.pScrollBarBackgroundBrush.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    hr = s_pRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(D2D1::ColorF::OrangeRed),
        s_RenderTargetResources.pScrollBarHighlightBrush.ReleaseAndGetAddressOf());
  }

  return hr;
}

void Main::OnResize(UINT width, UINT height)
{
  if (s_pRenderTarget)
  {
    MJ_UNINITIALIZED D2D1_SIZE_U size;
    size.width  = width;
    size.height = height;

    s_pRenderTarget->Resize(size);
  }

  s_TextEdit.Resize(width, height);
}

HMONITOR Main::GetPrimaryMonitor()
{
  return MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
}

void Main::SetWordWrap(bool wordWrap)
{
  if (s_pTextFormat)
  {
    s_pTextFormat->SetWordWrapping(wordWrap ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
  }
}

HRESULT Main::CreateDeviceResources()
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

      hr = s_pD2DFactory->CreateHwndRenderTarget(MJ_REF rtp, MJ_REF hrtp, s_pRenderTarget.ReleaseAndGetAddressOf());
    }

    if (SUCCEEDED(hr))
    {
      hr = CreateRenderTargetResources();
    }
  }

  if (SUCCEEDED(hr))
  {
    // Create a text layout using the text format.
    hr = s_TextEdit.CreateDeviceResources(s_pDWriteFactory, s_pTextFormat, (FLOAT)rect.right, (FLOAT)rect.bottom);
  }

  if (SUCCEEDED(hr))
  {
    SetWordWrap(s_WordWrap);
  }

  return hr;
}

static void DestroyRenderTargetResources(RenderTargetResources* pRenderTargetResources)
{
  pRenderTargetResources->pTextBrush.Reset();
  pRenderTargetResources->pTextEditBackgroundBrush.Reset();
  pRenderTargetResources->pScrollBarBrush.Reset();
  pRenderTargetResources->pCaretBrush.Reset();
  pRenderTargetResources->pScrollBarBackgroundBrush.Reset();
  pRenderTargetResources->pScrollBarHighlightBrush.Reset();
}

HRESULT Main::DrawD2DContent()
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
    s_pRenderTarget.Reset();
    DestroyRenderTargetResources(&s_RenderTargetResources);
  }

  return hr;
}

void Main::CalculateDpiScale()
{
  UINT dpi   = GetDpiForWindow(s_Hwnd);
  s_DpiScale = (float)dpi / MJ_96_DPI;
}

FLOAT Main::ConvertPointSizeToDIP(FLOAT points)
{
  return (points * MJ_POINT) * MJ_96_DPI;
}

HRESULT Main::CreateDeviceIndependentResources()
{
  HRESULT hr;

  // Create Direct2D factory.
#ifdef _DEBUG
  D2D1_FACTORY_OPTIONS options;
  options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, s_pD2DFactory.ReleaseAndGetAddressOf());
#else
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, s_pD2DFactory.ReleaseAndGetAddressOf());
#endif // _DEBUG

  // Create a shared DirectWrite factory.
  if (SUCCEEDED(hr))
  {
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(s_pDWriteFactory.ReleaseAndGetAddressOf()));
  }

  // This sets the default font, weight, stretch, style, and locale.
  if (SUCCEEDED(hr))
  {
    hr = s_pDWriteFactory->CreateTextFormat(
        L"Consolas", // Font family name.
        nullptr,     // Font collection (nullptr sets it to use the system font collection).
        DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        ConvertPointSizeToDIP(s_FontSize), L"en-us", s_pTextFormat.ReleaseAndGetAddressOf());
  }

  return hr;
}

HRESULT Main::BasicFileOpen()
{
  mj::ComPtr<IFileOpenDialog> pFileOpen;

  // Create the FileOpenDialog object.
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                reinterpret_cast<void**>(pFileOpen.GetAddressOf()));

  if (SUCCEEDED(hr))
  {
    // Show the Open dialog box.
    hr = pFileOpen->Show(nullptr);

    // Get the file name from the dialog box.
    if (SUCCEEDED(hr))
    {
      mj::ComPtr<IShellItem> pItem;
      hr = pFileOpen->GetResult(pItem.GetAddressOf());
      if (SUCCEEDED(hr))
      {
        PWSTR pszFilePath;
        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

        // Display the file name to the user.
        if (SUCCEEDED(hr))
        {
          MessageBoxW(nullptr, pszFilePath, L"File Path", MB_OK);
          CoTaskMemFree(pszFilePath);
        }
      }
    }
  }

  return hr;
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

  switch (message)
  {
  case WM_NCCREATE:
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)((CREATESTRUCT*)lParam)->lpCreateParams);
    SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
    break;
  case WM_ERASEBKGND: // don't want flicker
    return true;
  default:
    Main* pMain = (Main*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (pMain)
    {
      pMain->WndProc(hwnd, message, wParam, lParam);
    }
  }

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

HRESULT Main::RegisterWindowClass()
{
  HRESULT hr = S_OK;

  MJ_UNINITIALIZED WNDCLASSEX wcex;
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = ::WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = MAKEINTRESOURCEW(IDR_MENU1);
  wcex.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
  wcex.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
  wcex.lpszClassName = pWindowClassName;
  wcex.hIconSm       = LoadIconW(nullptr, IDI_APPLICATION);

  ATOM atom = RegisterClassExW(&wcex);
  if (!atom)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
  }

  return hr;
}

void Main::Start()
{
  HRESULT hr = S_OK;

  if (SUCCEEDED(hr))
  {
    hr = Main::RegisterWindowClass();
  }
  if (SUCCEEDED(hr))
  {
    hr = mj::TextEdit::RegisterWindowClass();
  }

  // We currently assume that the application will always be created on the primary monitor.
  UINT dpiX = 0;
  UINT dpiY = 0;

  if (SUCCEEDED(hr))
  {
    hr = GetDpiForMonitor(GetPrimaryMonitor(), MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
  }

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
    s_Hwnd = CreateWindowExW(0, pWindowClassName, L"Rekenaar", dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                             static_cast<int>(windowWidth), static_cast<int>(windowHeight), nullptr, nullptr,
                             HINST_THISCOMPONENT, this);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_TextEdit.Init(s_Hwnd, 20.0f, WINDOW_WIDTH / 2.0f, WINDOW_HEIGHT / 2.0f);
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

  // Load the accelerator table.
  auto hAccel = LoadAcceleratorsW(HINST_THISCOMPONENT, MAKEINTRESOURCE(IDR_ACCELERATOR1));

  if (SUCCEEDED(hr))
  {
    MSG msg;

    // Event loop
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      if (!TranslateAcceleratorW(s_Hwnd, hAccel, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
      }
    }
  }

  s_TextEdit.Destroy();
}

void Main::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case ID_FILE_OPEN:
      BasicFileOpen();
      break;
    case ID_VIEW_WORDWRAP:
      s_WordWrap = !s_WordWrap;
      CheckMenuItem(GetMenu(hwnd), ID_VIEW_WORDWRAP, s_WordWrap ? MF_CHECKED : MF_UNCHECKED);
      SetWordWrap(s_WordWrap);
      DrawD2DContent();
      break;
    default:
      break;
    }
    break;
  case WM_CAPTURECHANGED:
    s_TextEdit.MouseUp();
    break;
  case WM_MOUSEMOVE:
    s_TextEdit.MouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
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
    s_TextEdit.WndProc(hwnd, message, wParam, lParam);
    DrawD2DContent();
    break;
  case WM_KEYDOWN:
    switch (wParam)
    {
    case VK_HOME:
    case VK_END:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_DELETE:
    case VK_BACK:
      s_TextEdit.WndProc(hwnd, message, wParam, lParam);
      DrawD2DContent();
      break;
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
    s_pRenderTarget.Reset();
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
}

// CRT-less entry point
void CALLBACK WinMainCRTStartup() noexcept
{
  // The Microsoft Security Development Lifecycle recommends that all
  // applications include the following call to ensure that heap corruptions
  // do not go unnoticed and therefore do not introduce opportunities
  // for security exploits.
  MJ_DISCARD(HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
  if (SUCCEEDED(hr))
  {
    Main main;
    main.Start();
  }
  CoUninitialize();

  // CRT-less exit
  ExitProcess(0);
}
