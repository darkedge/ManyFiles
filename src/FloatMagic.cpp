#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "Common.h"
#include "DrawingEffect.h"
#include "RenderTarget.h"
#include "EditableLayout.h"
#include "TextEditor.h"
#include "FloatMagic.h"
#include "resource.h"

#include <shobjidl.h> // Save/Load dialogs
#include <shlobj.h>   // Save/Load dialogs

static void FailApplication(const wchar_t* message, int functionResult);

const static wchar_t g_sampleText[]     = L"Hello, world!\r\n";
static constexpr const wchar_t* s_pFont = L"Consolas";
static constexpr const FLOAT s_FontSize = 14;

static void Main()
{
  // The Microsoft Security Development Lifecycle recommends that all
  // applications include the following call to ensure that heap corruptions
  // do not go unnoticed and therefore do not introduce opportunities
  // for security exploits.
  static_cast<void>(HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

  HRESULT hr = CoInitialize(nullptr);
  if (SUCCEEDED(hr))
  {
    MainWindow app;

    hr = app.Initialize();
    if (SUCCEEDED(hr))
      hr = static_cast<HRESULT>(app.RunMessageLoop());
  }
  CoUninitialize();

  if (FAILED(hr))
  {
    FailApplication(L"An unexpected error occured in the demo. Ending now...", hr);
  }
}

#ifdef TRACY_ENABLE
// CRT entry point (required by Tracy)
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd)
{
  Main();

  return 0;
}
#else
void CALLBACK WinMainCRTStartup()
{
  Main();

  // CRT-less exit
  ExitProcess(0);
}
#endif

HRESULT MainWindow::Initialize()
{
  ZoneScoped;
  // Initializes the factories and creates the main window,
  // render target, and text editor.

  HRESULT hr = S_OK;

  if (SUCCEEDED(hr))
  {
    ZoneScopedN("DWriteCreateFactory");
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(dwriteFactory_.ReleaseAndGetAddressOf()));
  }

  if (SUCCEEDED(hr))
  {
    ZoneScopedN("D2D1CreateFactory");
    static_cast<void>(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.ReleaseAndGetAddressOf()));
  }

  // Create the main window
  if (SUCCEEDED(hr))
  {
    static_cast<void>(MainWindow::RegisterWindowClass());
    static_cast<void>(TextEditor::RegisterWindowClass());

    // create window (the hwnd is stored in the create event)
    ZoneScopedN("CreateWindowExW");
    CreateWindowExW(0L, MainWindow::kClassName, APPLICATION_TITLE, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                    CW_USEDEFAULT, 800, 600, nullptr, nullptr, HINST_THISCOMPONENT, this);

    if (!this->pHwnd)
      hr = HRESULT_FROM_WIN32(GetLastError());
  }

  // Need a text format to base the layout on.
  mj::ComPtr<IDWriteTextFormat> textFormat;
  if (SUCCEEDED(hr))
  {
    ZoneScopedN("CreateTextFormat");
    hr = dwriteFactory_->CreateTextFormat(s_pFont,                    //
                                          nullptr,                    //
                                          DWRITE_FONT_WEIGHT_NORMAL,  //
                                          DWRITE_FONT_STYLE_NORMAL,   //
                                          DWRITE_FONT_STRETCH_NORMAL, //
                                          s_FontSize,                 //
                                          L"",                        //
                                          textFormat.GetAddressOf());
  }

  // Set initial text and assign to the text editor.
  if (SUCCEEDED(hr))
  {
    hr = TextEditor::Create(this->pHwnd, g_sampleText, textFormat.Get(), dwriteFactory_.Get(), &this->pTextEditor);
  }

  if (SUCCEEDED(hr))
  {
    hr = this->FormatSampleLayout(this->pTextEditor->GetLayout());
  }

  // Create our target on behalf of text editor control
  // and tell it to draw onto it.
  if (SUCCEEDED(hr))
  {
    hr = this->CreateRenderTarget(this->pTextEditor->GetHwnd());
    this->pTextEditor->SetRenderTarget(this->pRenderTarget.Get());
  }

  if (SUCCEEDED(hr))
  {
    // Size everything initially.
    OnSize();

    // Put focus on editor to begin typing.
    static_cast<void>(SetFocus(this->pTextEditor->GetHwnd()));
  }

  // Initialize the controls
  if (SUCCEEDED(hr))
  {
    ZoneScopedN("ShowWindow");
    static_cast<void>(ShowWindow(this->pHwnd, SW_SHOWNORMAL));
    static_cast<void>(UpdateWindow(this->pHwnd));
  }

  return hr;
}

ATOM MainWindow::RegisterWindowClass()
{
  ZoneScoped;
  // Registers window class.
  MJ_UNINITIALIZED WNDCLASSEX wcex;
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_DBLCLKS;
  wcex.lpfnWndProc   = &WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hIcon         = nullptr;
  wcex.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = MAKEINTRESOURCEW(IDR_MENU1);
  wcex.lpszClassName = MainWindow::kClassName;
  wcex.hIconSm       = nullptr;

  return RegisterClassExW(&wcex);
}

HRESULT MainWindow::CreateRenderTarget(HWND hwnd)
{
  ZoneScoped;

  HRESULT hr = S_OK;

  mj::ComPtr<RenderTarget> renderTarget;

  // Create the render target.
  if (d2dFactory_)
  {
    hr = RenderTarget::Create(d2dFactory_.Get(), dwriteFactory_.Get(), hwnd, renderTarget.ReleaseAndGetAddressOf());
  }

  // Set the new target.
  if (SUCCEEDED(hr))
  {
    this->pRenderTarget = renderTarget;
  }

  return hr;
}

WPARAM MainWindow::RunMessageLoop()
{
  ZoneScoped;
  MJ_UNINITIALIZED MSG msg;
  MJ_UNINITIALIZED BOOL bRet;

  while ((bRet = GetMessageW(&msg, nullptr, 0, 0)) != 0)
  {
    if (bRet == -1)
    {
      // handle the error and possibly exit
    }
    else
    {
      static_cast<void>(TranslateMessage(&msg));
      static_cast<void>(DispatchMessageW(&msg));
    }
  }

  return msg.wParam;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  ZoneScoped;
  // Relays messages for the main window to the internal class.

  MainWindow* pMainWindow = reinterpret_cast<MainWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    // Associate the data structure with this window handle.
    CREATESTRUCT* pcs  = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow        = reinterpret_cast<MainWindow*>(pcs->lpCreateParams);
    pMainWindow->pHwnd = hwnd;
    static_cast<void>(SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));
  }
    return DefWindowProcW(hwnd, message, wParam, lParam);

  case WM_COMMAND:
    pMainWindow->OnCommand(static_cast<UINT>(wParam));
    break;

  case WM_SIZE:
    pMainWindow->OnSize();
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  case WM_NCDESTROY:
    break;

  case WM_SETFOCUS:
    // Forward focus to the text editor.
    if (pMainWindow->pTextEditor)
    {
      static_cast<void>(SetFocus(pMainWindow->pTextEditor->GetHwnd()));
    }
    break;

  case WM_INITMENU:
    // Menu about to be shown. Set check marks accordingly.
    pMainWindow->UpdateMenuToCaret();
    break;

  case WM_WINDOWPOSCHANGED:
    // Window moved. Update ClearType settings if changed monitor.
    if (pMainWindow->pRenderTarget)
    {
      pMainWindow->pRenderTarget->UpdateMonitor();
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);

  default:
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }

  return 0;
}

void MainWindow::OpenFileDialog()
{
  ZoneScoped;
  mj::ComPtr<IFileOpenDialog> pFileOpen;

  // Create the FileOpenDialog object.
  // TODO MJ: Create once?
  HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                reinterpret_cast<LPVOID*>(pFileOpen.GetAddressOf()));

  if (SUCCEEDED(hr))
  {
    hr = pFileOpen->Show(nullptr);
  }

  // Get the file name from the dialog box.
  mj::ComPtr<IShellItem> pItem;
  if (SUCCEEDED(hr))
  {
    hr = pFileOpen->GetResult(pItem.GetAddressOf());
  }

  PWSTR pszFilePath = nullptr;
  if (SUCCEEDED(hr))
  {
    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
  }

  if (SUCCEEDED(hr))
  {
    HANDLE hFile = CreateFileW(pszFilePath,           // file to open
                               GENERIC_READ,          // open for reading
                               FILE_SHARE_READ,       // share for reading
                               nullptr,               // default security
                               OPEN_EXISTING,         // existing file only
                               FILE_ATTRIBUTE_NORMAL, // normal file
                               nullptr);              // no attr. template

    if (hFile != INVALID_HANDLE_VALUE)
    {
      MJ_UNINITIALIZED DWORD dwBytesRead;
      constexpr DWORD BUFFERSIZE = 1024; // TODO MJ: Fixed buffer size
      char ReadBuffer[BUFFERSIZE];

      if (ReadFile(hFile, ReadBuffer, BUFFERSIZE - 1, &dwBytesRead, nullptr))
      {
        ReadBuffer[dwBytesRead] = '\0';

        // TODO MJ: Assuming default Windows ANSI code page
        const int cchWideChar =
            MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, ReadBuffer, dwBytesRead + 1, nullptr, 0);
        LPWSTR wide = reinterpret_cast<LPWSTR>(LocalAlloc(LMEM_FIXED, cchWideChar * sizeof(wchar_t)));
        if (MultiByteToWideChar(CP_ACP, MB_ERR_INVALID_CHARS, ReadBuffer, dwBytesRead + 1, wide, cchWideChar))
        {
          this->pTextEditor->SetText(wide);
        }
        else
        {
          // TODO: GetLastError
        }
      }
      else
      {
        MJ_UNINITIALIZED LPWSTR lpMsgBuf;
        const DWORD dw = GetLastError();

        // Allocation: FORMAT_MESSAGE_ALLOCATE_BUFFER (LocalAlloc) --> LocalFree
        // Note (Microsoft): LCID/LANGID/SORTID concept is deprecated, use Locale Names instead
        static_cast<void>(
            FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           nullptr, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&lpMsgBuf, 0, nullptr));
        static_cast<void>(MessageBoxW(nullptr, lpMsgBuf, TEXT("Error"), MB_OK));
        static_cast<void>(LocalFree(lpMsgBuf));
      }

      static_cast<void>(CloseHandle(hFile));
    }

    CoTaskMemFree(pszFilePath);
  }
}

void MainWindow::OnCommand(UINT commandId)
{
  ZoneScoped;
  // Handles menu commands.

  IDWriteTextLayout* textLayout = this->pTextEditor->GetLayout();

  switch (commandId)
  {
  case ID_EDIT_PASTETEXT:
    this->pTextEditor->PasteFromClipboard();
    break;

  case ID_EDIT_COPYTEXT:
    this->pTextEditor->CopyToClipboard();
    break;

  case ID_FORMAT_FONT:
    static_cast<void>(OnChooseFont());
    break;

  case ID_FORMAT_LEFT:
    static_cast<void>(textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT));
    RedrawTextEditor();
    break;

  case ID_FORMAT_RIGHT:
    static_cast<void>(textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT));
    RedrawTextEditor();
    break;

  case ID_FORMAT_WRAP:
  {
    const DWRITE_WORD_WRAPPING wordWrapping = textLayout->GetWordWrapping();
    static_cast<void>(textLayout->SetWordWrapping(
        (wordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP) ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP));
    RedrawTextEditor();
  }
  break;

  case ID_FORMAT_TRIM:
  {
    // Retrieve existing trimming sign and settings
    // and modify them according to button state.
    mj::ComPtr<IDWriteInlineObject> inlineObject;
    DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };

    static_cast<void>(textLayout->GetTrimming(&trimming, inlineObject.GetAddressOf()));
    trimming.granularity = (trimming.granularity == DWRITE_TRIMMING_GRANULARITY_NONE)
                               ? DWRITE_TRIMMING_GRANULARITY_CHARACTER
                               : DWRITE_TRIMMING_GRANULARITY_NONE;
    static_cast<void>(textLayout->SetTrimming(&trimming, inlineObject.Get()));

    RedrawTextEditor();
  }
  break;

  case ID_VIEW_ZOOMIN:
    this->pTextEditor->SetScale(1.25f, 1.25f, true);
    break;

  case ID_VIEW_Z:
    this->pTextEditor->SetScale(1 / 1.25f, 1 / 1.25f, true);
    break;

  case ID_FILE_EXIT:
    static_cast<void>(PostMessageW(this->pHwnd, WM_CLOSE, 0, 0));
    break;

  case ID_FILE_OPEN:
    OpenFileDialog();
    RedrawTextEditor();
    break;
  }

  return;
}

void MainWindow::OnSize()
{
  ZoneScoped;

  // Updates the child edit control's size to fill the whole window.

  if (!this->pTextEditor)
    return;

  RECT clientRect = {};
  static_cast<void>(GetClientRect(this->pHwnd, &clientRect));

  static_cast<void>(SetWindowPos(this->pTextEditor->GetHwnd(),       //
                                 nullptr,                            //
                                 clientRect.left,                    //
                                 clientRect.top,                     //
                                 clientRect.right - clientRect.left, //
                                 clientRect.bottom - clientRect.top, //
                                 SWP_NOACTIVATE | SWP_NOZORDER));
}

void MainWindow::RedrawTextEditor()
{
  ZoneScoped;
  // Flags text editor to redraw itself after significant changes.
  this->pTextEditor->RefreshView();
}

/// <summary>
/// Updates the menu state according to the formatting
/// at the current caret position.
/// </summary>
void MainWindow::UpdateMenuToCaret()
{
  ZoneScoped;
  // Read layout-wide attributes from the layout.
  const DWRITE_WORD_WRAPPING wordWrapping = this->pTextEditor->GetLayout()->GetWordWrapping();

  // Set checkbox/radio to true on certain menu items
  static_cast<void>(
      CheckMenuItem(GetMenu(this->pHwnd), ID_FORMAT_WRAP,
                    MF_BYCOMMAND | (wordWrapping != DWRITE_WORD_WRAPPING_NO_WRAP ? MF_CHECKED : MF_UNCHECKED)));
}

HRESULT MainWindow::OnChooseFont()
{
  ZoneScoped;
  // Displays the font selection dialog,
  // initializing it according to the current selection's format,
  // and updating the current selection with the user's choice.

  HRESULT hr = S_OK;

  // Read the caret format.
  EditableLayout::CaretFormat& caretFormat = this->pTextEditor->GetCaretFormat();

  // Initialize the LOGFONT from the caret format.
  LOGFONT logFont          = {};
  logFont.lfHeight         = -static_cast<LONG>(caretFormat.fontSize);
  logFont.lfWidth          = 0;
  logFont.lfEscapement     = 0;
  logFont.lfOrientation    = 0;
  logFont.lfWeight         = caretFormat.fontWeight;
  logFont.lfItalic         = (caretFormat.fontStyle > DWRITE_FONT_STYLE_NORMAL);
  logFont.lfUnderline      = static_cast<BYTE>(caretFormat.hasUnderline);
  logFont.lfStrikeOut      = static_cast<BYTE>(caretFormat.hasStrikethrough);
  logFont.lfCharSet        = DEFAULT_CHARSET;
  logFont.lfOutPrecision   = OUT_DEFAULT_PRECIS;
  logFont.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
  logFont.lfQuality        = DEFAULT_QUALITY;
  logFont.lfPitchAndFamily = DEFAULT_PITCH;
  if (SUCCEEDED(hr))
  {
    hr = ::StringCchCopyW(logFont.lfFaceName, ARRAYSIZE(logFont.lfFaceName), caretFormat.fontFamilyName);
  }

  // Initialize CHOOSEFONT for the dialog.

  CHOOSEFONT chooseFont  = {};
  chooseFont.lStructSize = sizeof(chooseFont);
  chooseFont.hwndOwner   = this->pHwnd;
  chooseFont.lpLogFont   = &logFont;
  chooseFont.iPointSize  = 120; // Note that LOGFONT initialization takes precedence anyway.
  chooseFont.rgbColors   = DrawingEffect::GetColorRef(caretFormat.color);
  chooseFont.Flags =
      CF_SCREENFONTS | CF_SCALABLEONLY | CF_NOVERTFONTS | CF_NOSCRIPTSEL | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT;
  // We don't show vertical fonts because we don't do vertical layout,
  // and don't show bitmap fonts because DirectWrite doesn't support them.

  // Show the common font dialog box.
  if (!ChooseFontW(&chooseFont))
    return hr; // user canceled.

  // Update the layout accordingly to what the user selected.

  // Abort if the user didn't select a face name.
  if (logFont.lfFaceName[0] == L'\0')
    return hr;

  mj::ComPtr<IDWriteFont> font;
  hr = CreateFontFromLOGFONT(logFont, font.GetAddressOf());

  if (SUCCEEDED(hr))
  {
    hr = GetFontFamilyName(font.Get(), caretFormat.fontFamilyName, ARRAYSIZE(caretFormat.fontFamilyName));
  }

  if (SUCCEEDED(hr))
  {
    caretFormat.hasUnderline     = logFont.lfUnderline;
    caretFormat.hasStrikethrough = logFont.lfStrikeOut;
    caretFormat.fontWeight       = font->GetWeight();
    caretFormat.fontStretch      = font->GetStretch();
    caretFormat.fontStyle        = font->GetStyle();
    caretFormat.fontSize         = mj::floorf(chooseFont.iPointSize * (96.0f / 720.0f));
    caretFormat.color            = DrawingEffect::GetBgra(chooseFont.rgbColors);

    const DWRITE_TEXT_RANGE textRange = this->pTextEditor->GetSelectionRange();
    if (textRange.length > 0)
    {
      IDWriteTextLayout* textLayout = this->pTextEditor->GetLayout();
      textLayout->SetUnderline(caretFormat.hasUnderline, textRange);
      textLayout->SetStrikethrough(caretFormat.hasStrikethrough, textRange);
      textLayout->SetFontWeight(caretFormat.fontWeight, textRange);
      textLayout->SetFontStretch(caretFormat.fontStretch, textRange);
      textLayout->SetFontStyle(caretFormat.fontStyle, textRange);
      textLayout->SetFontSize(caretFormat.fontSize, textRange);
      textLayout->SetFontFamilyName(caretFormat.fontFamilyName, textRange);

      DrawingEffect* drawingEffect = new DrawingEffect(caretFormat.color); // TODO MJ: Untracked memory allocation
      static_cast<void>(textLayout->SetDrawingEffect(drawingEffect, textRange));

      RedrawTextEditor();
    }
  }

  return hr;
}

STDMETHODIMP MainWindow::CreateFontFromLOGFONT(const LOGFONT& logFont, OUT IDWriteFont** font)
{
  ZoneScoped;
  *font = nullptr;

  // Conversion to and from LOGFONT uses the IDWriteGdiInterop interface.
  mj::ComPtr<IDWriteGdiInterop> gdiInterop;
  HRESULT hr = dwriteFactory_->GetGdiInterop(gdiInterop.GetAddressOf());

  // Find the font object that best matches the specified LOGFONT.
  if (SUCCEEDED(hr))
  {
    hr = gdiInterop->CreateFontFromLOGFONT(&logFont, font);
  }

  return hr;
}

STDMETHODIMP MainWindow::GetFontFamilyName(IDWriteFont* font, OUT wchar_t* fontFamilyName, UINT32 fontFamilyNameLength)
{
  ZoneScoped;
  // Get the font family to which this font belongs.
  mj::ComPtr<IDWriteFontFamily> fontFamily;
  HRESULT hr = font->GetFontFamily(fontFamily.GetAddressOf());

  // Get the family names. This returns an object that encapsulates one or
  // more names with the same meaning but in different languages.
  mj::ComPtr<IDWriteLocalizedStrings> localizedFamilyNames;
  if (SUCCEEDED(hr))
  {
    hr = fontFamily->GetFamilyNames(localizedFamilyNames.GetAddressOf());
  }

  // Get the family name at index zero. If we were going to display the name
  // we'd want to try to find one that matched the use locale, but for purposes
  // of setting the current font, any language will do.
  if (SUCCEEDED(hr))
  {
    hr = localizedFamilyNames->GetString(0, &fontFamilyName[0], fontFamilyNameLength);
  }

  return S_OK;
}

HRESULT MainWindow::FormatSampleLayout(IDWriteTextLayout* textLayout)
{
  ZoneScoped;

  // Formats the initial sample text with styles, drawing effects, and
  // typographic features.

  HRESULT hr = S_OK;

  // Set default color of black on entire range.
  DrawingEffect* drawingEffect = new DrawingEffect(0xFF000000); // TODO MJ: Untracked memory allocation
  if (SUCCEEDED(hr))
  {
    static_cast<void>(textLayout->SetDrawingEffect(drawingEffect, MakeDWriteTextRange(0)));
  }

  // Set initial trimming sign, but leave it disabled (granularity is none).

  mj::ComPtr<IDWriteInlineObject> inlineObject;
  if (SUCCEEDED(hr))
  {
    hr = dwriteFactory_->CreateEllipsisTrimmingSign(textLayout, inlineObject.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    static const DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };

    static_cast<void>(textLayout->SetTrimming(&trimming, inlineObject.Get()));
  }

  if (SUCCEEDED(hr))
  {
    static_cast<void>(textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT));
    static_cast<void>(textLayout->SetFontFamilyName(L"Segoe UI", MakeDWriteTextRange(0)));
    static_cast<void>(textLayout->SetFontSize(18, MakeDWriteTextRange(0)));
  }

  return hr;
}

void FailApplication(const wchar_t* message, int functionResult)
{
  ZoneScoped;
  // Displays an error message and quits the program.

  MJ_UNINITIALIZED wchar_t buffer[1000];
  buffer[0] = '\0';

  // const wchar_t* format = L"%s\r\nError code = %X";

  // StringCchPrintf(buffer, ARRAYSIZE(buffer), format, message, functionResult);
  static_cast<void>(MessageBoxW(nullptr, buffer, APPLICATION_TITLE, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL));
  ExitProcess(functionResult);
}
