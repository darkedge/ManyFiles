#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wrl/client.h>
#include <dwrite.h>

#include <d2d1.h>

// __ImageBase is better than GetCurrentModule()
// Can be cast to a HINSTANCE
#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

static HWND s_Hwnd;

// how much to scale a design that assumes 96-DPI pixels
static float dpiScaleX_;
static float dpiScaleY_;

// Direct2D
static Microsoft::WRL::ComPtr<ID2D1Factory> pD2DFactory;
static Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> pRT;
static Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pBlackBrush;

// DirectWrite
static Microsoft::WRL::ComPtr<IDWriteFactory> pDWriteFactory;
static Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;

static const wchar_t* wszText;
static UINT32 cTextLength;

void OnResize(UINT width, UINT height)
{
  if (pRT)
  {
    D2D1_SIZE_U size;
    size.width  = width;
    size.height = height;

    pRT->Resize(size);
  }
}

HRESULT CreateDeviceResources()
{
  HRESULT hr = S_OK;

  RECT rc;
  GetClientRect(s_Hwnd, &rc);

  D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

  if (!pRT)
  {
    // Create a Direct2D render target.
    hr = pD2DFactory->CreateHwndRenderTarget(
        D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(s_Hwnd, size), pRT.ReleaseAndGetAddressOf());

    // Create a black brush.
    if (SUCCEEDED(hr))
    {
      hr = pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), pBlackBrush.ReleaseAndGetAddressOf());
    }
  }

  return hr;
}

HRESULT TextDraw()
{
  RECT rc;

  GetClientRect(s_Hwnd, &rc);

  // Create a D2D rect that is the same size as the window.
  D2D1_RECT_F layoutRect = D2D1::RectF(
      static_cast<FLOAT>(rc.top) / dpiScaleY_, static_cast<FLOAT>(rc.left) / dpiScaleX_,
      static_cast<FLOAT>(rc.right - rc.left) / dpiScaleX_, static_cast<FLOAT>(rc.bottom - rc.top) / dpiScaleY_);

  // Use the DrawText method of the D2D render target interface to draw.
  pRT->DrawTextW(wszText, cTextLength, pTextFormat.Get(), layoutRect, pBlackBrush.Get());

  return S_OK;
}

HRESULT DrawD2DContent()
{
  HRESULT hr;

  hr = CreateDeviceResources();

  if (!(pRT->CheckWindowState() & D2D1_WINDOW_STATE_OCCLUDED))
  {
    pRT->BeginDraw();

    pRT->SetTransform(D2D1::IdentityMatrix());
    pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

    if (SUCCEEDED(hr))
    {
      TextDraw();
    }

    if (SUCCEEDED(hr))
    {
      hr = pRT->EndDraw();
    }
  }

  if (FAILED(hr))
  {
    pRT.Reset();
    pBlackBrush.Reset();
  }

  return hr;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_SIZE:
  {
    UINT width  = LOWORD(lParam);
    UINT height = HIWORD(lParam);
    OnResize(width, height);
  }
    return 0;

  case WM_PAINT:
  case WM_DISPLAYCHANGE:
  {
    ValidateRect(hwnd, nullptr);
    DrawD2DContent();
  }
    return 0;

  case WM_DESTROY:
  {
    PostQuitMessage(0);
  }
    return 1;
  }

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

FLOAT ConvertPointSizeToDIP(FLOAT points)
{
    return (points/72.0f)*96.0f;
}

// Called once.
HRESULT CreateDeviceIndependentResources()
{
  HRESULT hr;

  // Create Direct2D factory.
#ifdef _DEBUG
  D2D1_FACTORY_OPTIONS options;
  options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, options, pD2DFactory.ReleaseAndGetAddressOf());
#else
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pD2DFactory.ReleaseAndGetAddressOf());
#endif // _DEBUG

  // Create a shared DirectWrite factory.
  if (SUCCEEDED(hr))
  {
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(pDWriteFactory.ReleaseAndGetAddressOf()));
  }

  // The string to display.
  wszText     = L"Hello World using DirectWrite!";
  cTextLength = (UINT32)wcslen(wszText);

  // Create a text format using Gabriola with a font size of 72.
  // This sets the default font, weight, stretch, style, and locale.
  if (SUCCEEDED(hr))
  {
    hr = pDWriteFactory->CreateTextFormat(
        L"Consolas", // Font family name.
        nullptr,     // Font collection (nullptr sets it to use the system font collection).
        DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, ConvertPointSizeToDIP(11.0f), L"en-us",
        pTextFormat.ReleaseAndGetAddressOf());
  }

  // Center align (horizontally) the text.
  if (SUCCEEDED(hr))
  {
    hr = pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  }

  if (SUCCEEDED(hr))
  {
    hr = pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  }

  return hr;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)pCmdLine;
  (void)nCmdShow;

  WNDCLASSEX wcex;

  // get the dpi information
  HDC screen = GetDC(0);
  dpiScaleX_ = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
  dpiScaleY_ = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
  ReleaseDC(0, screen);

  // Return failure unless CreateDeviceIndependentResources returns SUCCEEDED.
  HRESULT hr = S_OK;

  ATOM atom;

  // Register window class.
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = WndProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = nullptr;
  wcex.hIcon         = LoadIconW(nullptr, IDI_APPLICATION);
  wcex.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
  wcex.lpszClassName = TEXT("DemoApp");
  wcex.hIconSm       = LoadIconW(nullptr, IDI_APPLICATION);

  atom = RegisterClassExW(&wcex);

  hr = atom ? S_OK : E_FAIL;

  if (SUCCEEDED(hr))
  {
    // Create window.
    s_Hwnd = CreateWindowExW(0, TEXT("DemoApp"), TEXT("Simple DirectWrite Hello World"), WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(640.0f / dpiScaleX_),
                            static_cast<int>(480.0f / dpiScaleY_), nullptr, nullptr, HINST_THISCOMPONENT, nullptr);
  }

  if (SUCCEEDED(hr))
  {
    hr = s_Hwnd ? S_OK : E_FAIL;
  }

  if (SUCCEEDED(hr))
  {
    hr = CreateDeviceIndependentResources();
  }

  if (SUCCEEDED(hr))
  {
    ShowWindow(s_Hwnd, SW_SHOWNORMAL);
    UpdateWindow(s_Hwnd);
  }

  if (SUCCEEDED(hr))
  {
    hr = DrawD2DContent();
  }

  if (SUCCEEDED(hr))
  {
    MSG msg;

    while (GetMessageW(&msg, nullptr, 0, 0))
    {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  return 0;
}
