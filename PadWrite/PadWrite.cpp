﻿#include "Common.h"
#include "DrawingEffect.h"
#include "RenderTarget.h"
#include "EditableLayout.h"
#include "TextEditor.h"
#include "PadWrite.h"
#include "resource.h"

static void FailApplication(const wchar_t* message, int functionResult);

const static wchar_t g_sampleText[] = L"Hello, world!\r\n";

////////////////////////////////////////
// Main entry.

void CALLBACK WinMainCRTStartup() noexcept
{
  // The Microsoft Security Development Lifecycle recommends that all
  // applications include the following call to ensure that heap corruptions
  // do not go unnoticed and therefore do not introduce opportunities
  // for security exploits.
  HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);

  HRESULT hr = S_OK;

  hr = CoInitialize(nullptr);
  if (SUCCEEDED(hr))
  {
    MainWindow app;
    app.AddRef(); // an implicit reference to the root window

    hr = app.Initialize();
    if (SUCCEEDED(hr))
      hr = static_cast<HRESULT>(app.RunMessageLoop());
  }
  CoUninitialize();

  if (FAILED(hr))
  {
    FailApplication(L"An unexpected error occured in the demo. Ending now...", hr);
  }

  // CRT-less exit
  ExitProcess(0);
}

MainWindow::MainWindow()
    : renderTargetType_(RenderTargetTypeD2D), //
      hwnd_(nullptr),                         //
      textEditor_()
{
  // no heavyweight initialization in the constructor.
}

HRESULT MainWindow::Initialize()
{
  // Initializes the factories and creates the main window,
  // render target, and text editor.

  HRESULT hr = S_OK;

  //////////////////////////////
  // Create the factories for D2D, DWrite, and WIC.

  if (SUCCEEDED(hr))
  {
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                             reinterpret_cast<IUnknown**>(dwriteFactory_.ReleaseAndGetAddressOf()));
  }

  // Create D2D factory
  // Failure to create this factory is ok. We can live with GDI alone.
  if (SUCCEEDED(hr))
  {
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2dFactory_.ReleaseAndGetAddressOf());
  }

  //////////////////////////////
  // Create the main window

  if (SUCCEEDED(hr))
  {
    MainWindow::RegisterWindowClass();
    TextEditor::RegisterWindowClass();

    // create window (the hwnd is stored in the create event)
    CreateWindow(L"DirectWritePadDemo", TEXT(APPLICATION_TITLE), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                 CW_USEDEFAULT, 800, 600, nullptr, nullptr, HINST_THISCOMPONENT, this);

    if (hwnd_ == nullptr)
      hr = HRESULT_FROM_WIN32(GetLastError());
  }

  //////////////////////////////
  // Initialize the controls

  if (SUCCEEDED(hr))
  {
    ShowWindow(hwnd_, SW_SHOWNORMAL);
    UpdateWindow(hwnd_);
  }

  // Need a text format to base the layout on.
  IDWriteTextFormat* textFormat = nullptr;
  if (SUCCEEDED(hr))
  {
    hr = dwriteFactory_->CreateTextFormat(L"Arial", nullptr, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                          DWRITE_FONT_STRETCH_NORMAL, 16, L"", &textFormat);
  }

  // Set initial text and assign to the text editor.
  if (SUCCEEDED(hr))
  {
    hr = TextEditor::Create(hwnd_, g_sampleText, textFormat, dwriteFactory_.Get(), textEditor_.ReleaseAndGetAddressOf());
  }

  if (SUCCEEDED(hr))
  {
    hr = FormatSampleLayout(textEditor_->GetLayout());
  }

  // Create our target on behalf of text editor control
  // and tell it to draw onto it.
  if (SUCCEEDED(hr))
  {
    hr = CreateRenderTarget(textEditor_->GetHwnd(), RenderTargetTypeD2D);
    textEditor_->SetRenderTarget(renderTarget_.Get());
  }

  if (SUCCEEDED(hr))
  {
    // Size everything initially.
    OnSize();

    // Put focus on editor to begin typing.
    SetFocus(textEditor_->GetHwnd());
  }

  SafeRelease(&textFormat);

  return hr;
}

ATOM MainWindow::RegisterWindowClass()
{
  // Registers window class.
  WNDCLASSEX wcex;
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_DBLCLKS;
  wcex.lpfnWndProc   = &WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hIcon         = nullptr;
  wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = MAKEINTRESOURCE(IDR_MENU1);
  wcex.lpszClassName = TEXT("DirectWritePadDemo");
  wcex.hIconSm       = nullptr;

  return RegisterClassEx(&wcex);
}

HRESULT MainWindow::CreateRenderTarget(HWND hwnd, RenderTargetType renderTargetType)
{
  // Creates a render target, either a D2D surface or DirectWrite GDI DIB.

  HRESULT hr = S_OK;

  mj::ComPtr<RenderTarget> renderTarget;

  // Create the render target.
  switch (renderTargetType)
  {
  case RenderTargetTypeD2D:
    if (d2dFactory_)
    {
      hr = RenderTargetD2D::Create(d2dFactory_.Get(), dwriteFactory_.Get(), hwnd, renderTarget.ReleaseAndGetAddressOf());
      break;
    }
  }

  // Set the new target.
  if (SUCCEEDED(hr))
  {
    renderTarget_ = renderTarget;
    renderTargetType_ = renderTargetType;
  }

  return hr;
}

WPARAM MainWindow::RunMessageLoop()
{
  MSG msg;
  while (GetMessage(&msg, nullptr, 0, 0) > 0)
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // Relays messages for the main window to the internal class.

  MainWindow* window = reinterpret_cast<MainWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    // Associate the data structure with this window handle.
    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    window            = reinterpret_cast<MainWindow*>(pcs->lpCreateParams);
    window->hwnd_     = hwnd;
    window->AddRef(); // implicit reference via HWND
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)window);
  }
    return DefWindowProc(hwnd, message, wParam, lParam);

  case WM_COMMAND:
    window->OnCommand(static_cast<UINT>(wParam));
    break;

  case WM_SIZE:
    window->OnSize();
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    window->OnDestroy();
    break;

  case WM_NCDESTROY:
    // Remove implicit reference via HWND.
    // After this, the window and data structure no longer exist.
    window->Release();
    break;

  case WM_SETFOCUS:
    // Forward focus to the text editor.
    if (window->textEditor_)
      SetFocus(window->textEditor_->GetHwnd());
    break;

  case WM_INITMENU:
    // Menu about to be shown. Set check marks accordingly.
    window->UpdateMenuToCaret();
    break;

  case WM_WINDOWPOSCHANGED:
    // Window moved. Update ClearType settings if changed monitor.
    if (window->renderTarget_)
      window->renderTarget_->UpdateMonitor();

    return DefWindowProc(hwnd, message, wParam, lParam);

  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  return 0;
}

void MainWindow::OnCommand(UINT commandId)
{
  // Handles menu commands.

  IDWriteTextLayout* textLayout = textEditor_->GetLayout();

  switch (commandId)
  {
  case ID_EDIT_PASTETEXT:
    textEditor_->PasteFromClipboard();
    break;

  case ID_EDIT_COPYTEXT:
    textEditor_->CopyToClipboard();
    break;

  case ID_FORMAT_FONT:
    OnChooseFont();
    break;

  case ID_FORMAT_LEFT:
    textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    RedrawTextEditor();
    break;

  case ID_FORMAT_RIGHT:
    textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_RIGHT_TO_LEFT);
    RedrawTextEditor();
    break;

  case ID_FORMAT_WRAP:
  {
    DWRITE_WORD_WRAPPING wordWrapping = textLayout->GetWordWrapping();
    textLayout->SetWordWrapping((wordWrapping == DWRITE_WORD_WRAPPING_NO_WRAP) ? DWRITE_WORD_WRAPPING_WRAP
                                                                               : DWRITE_WORD_WRAPPING_NO_WRAP);
    RedrawTextEditor();
  }
  break;

  case ID_FORMAT_TRIM:
  {
    // Retrieve existing trimming sign and settings
    // and modify them according to button state.
    IDWriteInlineObject* inlineObject = nullptr;
    DWRITE_TRIMMING trimming          = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };

    textLayout->GetTrimming(&trimming, &inlineObject);
    trimming.granularity = (trimming.granularity == DWRITE_TRIMMING_GRANULARITY_NONE)
                               ? DWRITE_TRIMMING_GRANULARITY_CHARACTER
                               : DWRITE_TRIMMING_GRANULARITY_NONE;
    textLayout->SetTrimming(&trimming, inlineObject);
    SafeRelease(&inlineObject);

    RedrawTextEditor();
  }
  break;

  case ID_VIEW_ZOOMIN:
    textEditor_->SetScale(1.25f, 1.25f, true);
    break;

  case ID_VIEW_Z:
    textEditor_->SetScale(1 / 1.25f, 1 / 1.25f, true);
    break;

  case ID_FILE_EXIT:
    PostMessage(hwnd_, WM_CLOSE, 0, 0);
    break;
  }

  return;
}

void MainWindow::OnSize()
{
  // Updates the child edit control's size to fill the whole window.

  if (!textEditor_)
    return;

  RECT clientRect = {};
  GetClientRect(hwnd_, &clientRect);

  SetWindowPos(textEditor_->GetHwnd(), nullptr, clientRect.left, clientRect.top, clientRect.right - clientRect.left,
               clientRect.bottom - clientRect.top, SWP_NOACTIVATE | SWP_NOZORDER);
}

void MainWindow::OnDestroy()
{
}

void MainWindow::RedrawTextEditor()
{
  // Flags text editor to redraw itself after significant changes.

  textEditor_->RefreshView();
}

/// <summary>
/// Updates the menu state according to the formatting
/// at the current caret position.
/// </summary>
void MainWindow::UpdateMenuToCaret()
{
  // Read layout-wide attributes from the layout.
  DWRITE_WORD_WRAPPING wordWrapping = textEditor_->GetLayout()->GetWordWrapping();

  // Set checkbox/radio to true on certain menu items
  CheckMenuItem(GetMenu(hwnd_), ID_FORMAT_WRAP,
                MF_BYCOMMAND | (wordWrapping != DWRITE_WORD_WRAPPING_NO_WRAP ? MF_CHECKED : MF_UNCHECKED));
}

HRESULT MainWindow::OnChooseFont()
{
  // Displays the font selection dialog,
  // initializing it according to the current selection's format,
  // and updating the current selection with the user's choice.

  HRESULT hr = S_OK;

  //////////////////////////////
  // Read the caret format.
  EditableLayout::CaretFormat& caretFormat = textEditor_->GetCaretFormat();

  //////////////////////////////
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
  StringCchCopy(logFont.lfFaceName, ARRAYSIZE(logFont.lfFaceName), caretFormat.fontFamilyName);

  //////////////////////////////
  // Initialize CHOOSEFONT for the dialog.

  CHOOSEFONT chooseFont  = {};
  chooseFont.lStructSize = sizeof(chooseFont);
  chooseFont.hwndOwner   = hwnd_;
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

  //////////////////////////////
  // Update the layout accordingly to what the user selected.

  // Abort if the user didn't select a face name.
  if (logFont.lfFaceName[0] == L'\0')
    return hr;

  IDWriteFont* font = nullptr;
  hr                = CreateFontFromLOGFONT(logFont, &font);

  if (SUCCEEDED(hr))
  {
    hr = GetFontFamilyName(font, caretFormat.fontFamilyName, ARRAYSIZE(caretFormat.fontFamilyName));
  }

  if (SUCCEEDED(hr))
  {
    caretFormat.hasUnderline     = logFont.lfUnderline;
    caretFormat.hasStrikethrough = logFont.lfStrikeOut;
    caretFormat.fontWeight       = font->GetWeight();
    caretFormat.fontStretch      = font->GetStretch();
    caretFormat.fontStyle        = font->GetStyle();
    caretFormat.fontSize         = mj::floorf(float(chooseFont.iPointSize * (96.0f / 720)));
    caretFormat.color            = DrawingEffect::GetBgra(chooseFont.rgbColors);

    DWRITE_TEXT_RANGE textRange = textEditor_->GetSelectionRange();
    if (textRange.length > 0)
    {
      IDWriteTextLayout* textLayout = textEditor_->GetLayout();
      textLayout->SetUnderline(caretFormat.hasUnderline, textRange);
      textLayout->SetStrikethrough(caretFormat.hasStrikethrough, textRange);
      textLayout->SetFontWeight(caretFormat.fontWeight, textRange);
      textLayout->SetFontStretch(caretFormat.fontStretch, textRange);
      textLayout->SetFontStyle(caretFormat.fontStyle, textRange);
      textLayout->SetFontSize(caretFormat.fontSize, textRange);
      textLayout->SetFontFamilyName(caretFormat.fontFamilyName, textRange);

      DrawingEffect* drawingEffect = SafeAcquire(new DrawingEffect(caretFormat.color));
      textLayout->SetDrawingEffect(drawingEffect, textRange);
      SafeRelease(&drawingEffect);

      RedrawTextEditor();
    }
  }

  SafeRelease(&font);

  return hr;
}

STDMETHODIMP MainWindow::CreateFontFromLOGFONT(const LOGFONT& logFont, OUT IDWriteFont** font)
{
  *font = nullptr;

  HRESULT hr = S_OK;

  // Conversion to and from LOGFONT uses the IDWriteGdiInterop interface.
  IDWriteGdiInterop* gdiInterop = nullptr;
  hr                            = dwriteFactory_->GetGdiInterop(&gdiInterop);

  // Find the font object that best matches the specified LOGFONT.
  if (SUCCEEDED(hr))
  {
    hr = gdiInterop->CreateFontFromLOGFONT(&logFont, font);
  }

  SafeRelease(&gdiInterop);

  return hr;
}

STDMETHODIMP MainWindow::GetFontFamilyName(IDWriteFont* font, OUT wchar_t* fontFamilyName, UINT32 fontFamilyNameLength)
{
  HRESULT hr = S_OK;

  // Get the font family to which this font belongs.
  IDWriteFontFamily* fontFamily = nullptr;
  hr                            = font->GetFontFamily(&fontFamily);

  // Get the family names. This returns an object that encapsulates one or
  // more names with the same meaning but in different languages.
  IDWriteLocalizedStrings* localizedFamilyNames = nullptr;
  if (SUCCEEDED(hr))
  {
    hr = fontFamily->GetFamilyNames(&localizedFamilyNames);
  }

  // Get the family name at index zero. If we were going to display the name
  // we'd want to try to find one that matched the use locale, but for purposes
  // of setting the current font, any language will do.
  if (SUCCEEDED(hr))
  {
    hr = localizedFamilyNames->GetString(0, &fontFamilyName[0], fontFamilyNameLength);
  }

  SafeRelease(&localizedFamilyNames);
  SafeRelease(&fontFamily);

  return S_OK;
}

HRESULT MainWindow::FormatSampleLayout(IDWriteTextLayout* textLayout)
{
  // Formats the initial sample text with styles, drawing effects, and
  // typographic features.

  HRESULT hr = S_OK;

  // Set default color of black on entire range.
  DrawingEffect* drawingEffect = SafeAcquire(new DrawingEffect(0xFF000000));
  if (SUCCEEDED(hr))
  {
    textLayout->SetDrawingEffect(drawingEffect, MakeDWriteTextRange(0));
  }

  // Set initial trimming sign, but leave it disabled (granularity is none).

  IDWriteInlineObject* inlineObject = nullptr;
  if (SUCCEEDED(hr))
  {
    hr = dwriteFactory_->CreateEllipsisTrimmingSign(textLayout, &inlineObject);
  }

  if (SUCCEEDED(hr))
  {
    const static DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
    textLayout->SetTrimming(&trimming, inlineObject);
  }

  if (SUCCEEDED(hr))
  {
    textLayout->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);

    textLayout->SetFontFamilyName(L"Segoe UI", MakeDWriteTextRange(0));
    textLayout->SetFontSize(18, MakeDWriteTextRange(0));

#if 0
    // Apply a color to the title words.
    {
      DrawingEffect* drawingEffect1 = SafeAcquire(new DrawingEffect(0xFF1010D0));
      DrawingEffect* drawingEffect2 = SafeAcquire(new DrawingEffect(0xFF10D010));
      textLayout->SetDrawingEffect(drawingEffect1, MakeDWriteTextRange(0, 7));
      textLayout->SetDrawingEffect(drawingEffect2, MakeDWriteTextRange(7, 5));
      SafeRelease(&drawingEffect2);
      SafeRelease(&drawingEffect1);
    }

    // Set title font style.
    textLayout->SetFontSize(30, MakeDWriteTextRange(0, 25)); // first line of text
    textLayout->SetFontSize(60, MakeDWriteTextRange(1, 11)); // DirectWrite
    textLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, MakeDWriteTextRange(0, 25));
    textLayout->SetFontFamilyName(L"Gabriola", MakeDWriteTextRange(1, 11));

    // Add fancy swashes.
    {
      IDWriteTypography* typoFeature = nullptr;
      dwriteFactory_->CreateTypography(&typoFeature);
      DWRITE_FONT_FEATURE feature = { DWRITE_FONT_FEATURE_TAG_STYLISTIC_SET_7, 1 };
      typoFeature->AddFontFeature(feature);
      textLayout->SetTypography(typoFeature, MakeDWriteTextRange(1, 11));
      SafeRelease(&typoFeature);
    }

    // Apply decorations on demonstrated features.
    textLayout->SetFontWeight(DWRITE_FONT_WEIGHT_BOLD, MakeDWriteTextRange(277, 4));
    textLayout->SetFontStyle(DWRITE_FONT_STYLE_ITALIC, MakeDWriteTextRange(282, 6));
    textLayout->SetUnderline(TRUE, MakeDWriteTextRange(289, 9));
    textLayout->SetStrikethrough(TRUE, MakeDWriteTextRange(299, 13));
    textLayout->SetFontFamilyName(L"Arial", MakeDWriteTextRange(313, 6));
    textLayout->SetFontStretch(DWRITE_FONT_STRETCH_CONDENSED, MakeDWriteTextRange(313, 6));
    textLayout->SetFontWeight(DWRITE_FONT_WEIGHT_LIGHT, MakeDWriteTextRange(320, 5));

    // Localized S with comma below.
    textLayout->SetFontFamilyName(L"Tahoma", MakeDWriteTextRange(507, 3));
    textLayout->SetFontSize(16, MakeDWriteTextRange(507, 3));
    textLayout->SetFontFamilyName(L"Tahoma", MakeDWriteTextRange(514, 3));
    textLayout->SetFontSize(16, MakeDWriteTextRange(514, 3));
    textLayout->SetLocaleName(L"ro-ro", MakeDWriteTextRange(514, 3));

    // Localized forms of extended Arabic-Indic numbers 4 & 6, Pakistan Urdu.
    textLayout->SetFontFamilyName(L"Tahoma", MakeDWriteTextRange(519, 2));
    textLayout->SetFontFamilyName(L"Tahoma", MakeDWriteTextRange(525, 2));
    textLayout->SetLocaleName(L"ur-PK", MakeDWriteTextRange(525, 2));
#endif
  }

  SafeRelease(&inlineObject);
  SafeRelease(&drawingEffect);

  return hr;
}

void FailApplication(const wchar_t* message, int functionResult)
{
  // Displays an error message and quits the program.

  wchar_t buffer[1000];
  buffer[0] = '\0';

  // const wchar_t* format = L"%s\r\nError code = %X";

  // StringCchPrintf(buffer, ARRAYSIZE(buffer), format, message, functionResult);
  MessageBoxW(nullptr, buffer, TEXT(APPLICATION_TITLE), MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL);
  ExitProcess(functionResult);
}