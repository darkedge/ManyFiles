#include "..\3rdparty\tracy\Tracy.hpp"
#include "Common.h"
#include "DrawingEffect.h"
#include "RenderTarget.h"
#include "EditableLayout.h"
#include "TextEditor.h"

static inline D2D1::Matrix3x2F& Cast(DWRITE_MATRIX& matrix)
{
  // DWrite's matrix, D2D's matrix, and GDI's XFORM
  // are all compatible.
  return *reinterpret_cast<D2D1::Matrix3x2F*>(&matrix);
}

static inline DWRITE_MATRIX& Cast(D2D1::Matrix3x2F& matrix)
{
  return *reinterpret_cast<DWRITE_MATRIX*>(&matrix);
}

static inline int32_t RoundToInt(float x)
{
  return static_cast<int32_t>(mj::floorf(x + 0.5f));
}

static inline float DegreesToRadians(float degrees)
{
  return degrees * mj::kPi * 2.0f / 360.0f;
}

static inline float GetDeterminant(DWRITE_MATRIX const& matrix)
{
  return matrix.m11 * matrix.m22 - matrix.m12 * matrix.m21;
}

static void ComputeInverseMatrix(DWRITE_MATRIX const& matrix, OUT DWRITE_MATRIX& result)
{
  // Used for hit-testing, mouse scrolling, panning, and scroll bar sizing.

  const float invdet = 1.0f / GetDeterminant(matrix);
  result.m11         = matrix.m22 * invdet;
  result.m12         = -matrix.m12 * invdet;
  result.m21         = -matrix.m21 * invdet;
  result.m22         = matrix.m11 * invdet;
  result.dx          = (matrix.m21 * matrix.dy - matrix.dx * matrix.m22) * invdet;
  result.dy          = (matrix.dx * matrix.m12 - matrix.m11 * matrix.dy) * invdet;
}

static D2D1_POINT_2F GetPageSize(IDWriteTextLayout* textLayout)
{
  // Use the layout metrics to determine how large the page is, taking
  // the maximum of the content size and layout's maximal dimensions.

  MJ_UNINITIALIZED DWRITE_TEXT_METRICS textMetrics;
  textLayout->GetMetrics(&textMetrics);

  const float width  = mj_max(textMetrics.layoutWidth, textMetrics.left + textMetrics.width);
  const float height = mj_max(textMetrics.layoutHeight, textMetrics.height);

  D2D1_POINT_2F pageSize = { width, height };
  return pageSize;
}

static bool IsLandscapeAngle(float angle)
{
  return false;
}

// Initialization.

ATOM TextEditor::RegisterWindowClass()
{
  ZoneScoped;

  // Registers window class.
  MJ_UNINITIALIZED WNDCLASSEX wcex;
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = &WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hIcon         = nullptr;
  wcex.hCursor       = LoadCursorW(nullptr, IDC_IBEAM);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = nullptr;
  wcex.lpszClassName = TextEditor::kClassName;
  wcex.hIconSm       = nullptr;

  return RegisterClassExW(&wcex);
}

TextEditor::TextEditor(IDWriteFactory* factory)
    : layoutEditor_(factory),                  //
      allocator(nullptr, mj::Win32Alloc, mj::Win32Free), //
      text_(&allocator)
{
  ZoneScoped;

  // TODO MJ: Untracked memory allocations
  pageBackgroundEffect_        = new DrawingEffect(0xFF000000 | D2D1::ColorF::White);
  textSelectionEffect_         = new DrawingEffect(0xFF000000 | D2D1::ColorF::LightSkyBlue);
  imageSelectionEffect_        = new DrawingEffect(0x80000000 | D2D1::ColorF::LightSkyBlue);
  this->caretBackgroundEffect_ = new DrawingEffect(0xFF000000 | D2D1::ColorF::Black);

  // Creates editor window.

  this->InitDefaults();
  this->InitViewDefaults();
}

HRESULT TextEditor::Create(HWND parentHwnd, const wchar_t* text, IDWriteTextFormat* textFormat, IDWriteFactory* factory,
                           OUT TextEditor** textEditor)
{
  ZoneScoped;

  *textEditor = nullptr;
  HRESULT hr  = S_OK;

  // Create and initialize.
  TextEditor* newTextEditor = new TextEditor(factory); // TODO MJ: Untracked memory allocation
  if (!newTextEditor)
  {
    return E_OUTOFMEMORY;
  }

  hr = newTextEditor->Initialize(parentHwnd, text, textFormat);
  if (FAILED(hr))
    delete newTextEditor;

  *textEditor = newTextEditor;

  return hr;
}

HRESULT TextEditor::Initialize(HWND parentHwnd, const wchar_t* text, IDWriteTextFormat* textFormat)
{
  ZoneScoped;

  HRESULT hr = S_OK;

  // Set the initial text.
  MJ_UNINITIALIZED size_t length;
  if (SUCCEEDED(hr))
  {
    hr = ::StringCchLengthW(text, 1024, &length); // TODO MJ: Fixed buffer size
  }
  this->text_.Assign(text, static_cast<uint32_t>(length));
  this->text_.Add('\0');

  // Create an ideal layout for the text editor based on the text and format,
  // favoring document layout over pixel alignment.
  hr = layoutEditor_.GetFactory()->CreateTextLayout(this->text_.begin(), static_cast<UINT32>(length), textFormat,
                                                    580, // TODO MJ: maximum width
                                                    420, // TODO MJ: maximum height
                                                    this->pTextLayout.ReleaseAndGetAddressOf());

  if (FAILED(hr))
    return hr;

  // Get size of text layout; needed for setting the view origin.
  const float layoutWidth  = this->pTextLayout->GetMaxWidth();
  const float layoutHeight = this->pTextLayout->GetMaxHeight();
  this->originX_           = layoutWidth / 2;
  this->originY_           = layoutHeight / 2;

  // Set the initial text layout and update caret properties accordingly.
  UpdateCaretFormatting();

  // Create text editor window (hwnd is stored in the create event)
  static_cast<void>(CreateWindowExW(WS_EX_STATICEDGE, TextEditor::kClassName, L"",
                                    WS_CHILDWINDOW | WS_VSCROLL | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
                                    CW_USEDEFAULT, CW_USEDEFAULT, parentHwnd, nullptr, HINST_THISCOMPONENT, this));
  if (!this->hwnd_)
    return HRESULT_FROM_WIN32(GetLastError());

  return S_OK;
}

inline void TextEditor::InitDefaults()
{
  this->hwnd_ = nullptr;

  this->caretPosition_       = 0;
  this->caretAnchor_         = 0;
  this->caretPositionOffset_ = 0;

  this->currentlySelecting_ = false;
  this->currentlyPanning_   = false;
  this->previousMouseX      = 0;
  this->previousMouseY      = 0;
}

inline void TextEditor::InitViewDefaults()
{
  this->scaleX_  = 1.0f;
  this->scaleY_  = 1.0f;
  this->angle_   = 0.0f;
  this->originX_ = 0.0f;
  this->originY_ = 0.0f;
}

void TextEditor::SetRenderTarget(RenderTarget* target)
{
  ZoneScoped;

  this->renderTarget_ = target;
  this->PostRedraw();
}

// Message dispatch.

LRESULT CALLBACK TextEditor::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  ZoneScoped;

  // Relays messages for the text editor to the internal class.

  TextEditor* pWindow = reinterpret_cast<TextEditor*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    // Associate the data structure with this window handle.
    const CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    pWindow                 = reinterpret_cast<TextEditor*>(pcs->lpCreateParams);
    pWindow->hwnd_          = hwnd;
    static_cast<void>(SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow)));

    return DefWindowProc(hwnd, message, wParam, lParam);
  }

  case WM_PAINT:
  case WM_DISPLAYCHANGE:
    pWindow->OnDraw();
    break;

  case WM_ERASEBKGND: // don't want flicker
    return true;

  case WM_KEYDOWN:
    pWindow->OnKeyPress(static_cast<UINT>(wParam));
    break;

  case WM_CHAR:
    pWindow->OnKeyCharacter(static_cast<UINT>(wParam));
    break;

  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
    SetFocus(hwnd);
    SetCapture(hwnd);
    pWindow->OnMousePress(message, static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
    break;

  case WM_MOUSELEAVE:
  case WM_CAPTURECHANGED:
    pWindow->OnMouseExit();
    break;

  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
    ReleaseCapture();
    pWindow->OnMouseRelease(message, static_cast<float>(GET_X_LPARAM(lParam)),
                            static_cast<float>(GET_Y_LPARAM(lParam)));
    break;

  case WM_SETFOCUS:
  {
    D2D1_RECT_F rect;
    pWindow->GetCaretRect(rect);
    pWindow->UpdateSystemCaret(rect);
  }
  break;

  case WM_KILLFOCUS:
    DestroyCaret();
    break;

  case WM_MOUSEMOVE:
    pWindow->OnMouseMove(static_cast<float>(GET_X_LPARAM(lParam)), static_cast<float>(GET_Y_LPARAM(lParam)));
    break;

  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
  {
    // Retrieve the lines-to-scroll or characters-to-scroll user setting,
    // using a default value if the API failed.
    UINT userSetting;
    const BOOL success = SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &userSetting, 0);
    if (success == FALSE)
      userSetting = 1;

    // Set x,y scroll difference,
    // depending on whether horizontal or vertical scroll.
    const float zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    float yScroll      = (zDelta / WHEEL_DELTA) * userSetting;
    float xScroll      = 0;
    if (message == WM_MOUSEHWHEEL)
    {
      xScroll = -yScroll;
      yScroll = 0;
    }

    pWindow->OnMouseScroll(xScroll, yScroll);
  }
  break;

  case WM_VSCROLL:
  case WM_HSCROLL:
    pWindow->OnScroll(message, LOWORD(wParam));
    break;

  case WM_SIZE:
  {
    const UINT width  = LOWORD(lParam);
    const UINT height = HIWORD(lParam);
    pWindow->OnSize(width, height);
  }
  break;

  default:
    return DefWindowProcW(hwnd, message, wParam, lParam);
  }

  return 0;
}

// Drawing/scrolling/sizing.

void TextEditor::OnDraw()
{
  MJ_UNINITIALIZED PAINTSTRUCT ps;
  static_cast<void>(::BeginPaint(this->hwnd_, &ps));

  if (this->renderTarget_) // in case event received before we have a target
  {
    this->renderTarget_->BeginDraw();
    this->renderTarget_->Clear(D2D1::ColorF::LightGray);
    this->DrawPage(*this->renderTarget_);
    this->renderTarget_->EndDraw();
  }
  static_cast<void>(::EndPaint(this->hwnd_, &ps));
}

void TextEditor::DrawPage(RenderTarget& target)
{
  // Draws the background, page, selection, and text.

  // Calculate actual location in render target based on the
  // current page transform and location of edit control.
  MJ_INITIALIZED D2D1::Matrix3x2F pageTransform;
  this->GetViewMatrix(&Cast(pageTransform));

  // Scale/Rotate canvas as needed
  MJ_UNINITIALIZED DWRITE_MATRIX previousTransform;
  target.GetTransform(previousTransform);
  target.SetTransform(Cast(pageTransform));

  // Draw the page
  const D2D1_POINT_2F pageSize = GetPageSize(this->pTextLayout.Get());
  D2D1_RECT_F pageRect         = { 0, 0, pageSize.x, pageSize.y };

  target.FillRectangle(pageRect, *pageBackgroundEffect_.Get());

  // Determine actual number of hit-test ranges
  const DWRITE_TEXT_RANGE caretRange = GetSelectionRange();
  UINT32 actualHitTestCount          = 0;

  if (caretRange.length > 0)
  {
    this->pTextLayout->HitTestTextRange(caretRange.startPosition, caretRange.length,
                                        0, // x
                                        0, // y
                                        nullptr,
                                        0, // metrics count
                                        &actualHitTestCount);
  }

  // Allocate enough room to return all hit-test metrics.
  // MJ TODO: Use pre-allocated memory
  mj::Allocator allocator(nullptr, mj::Win32Alloc, mj::Win32Free);
  mj::ArrayList<DWRITE_HIT_TEST_METRICS> hitTestMetrics(&allocator, actualHitTestCount);
  hitTestMetrics.Reserve(actualHitTestCount);

  if (caretRange.length > 0)
  {
    this->pTextLayout->HitTestTextRange(caretRange.startPosition, caretRange.length,
                                        0, // x
                                        0, // y
                                        &hitTestMetrics[0], static_cast<UINT32>(hitTestMetrics.Size()),
                                        &actualHitTestCount);
  }

  // Draw the selection ranges behind the text.
  if (actualHitTestCount > 0)
  {
    // Note that an ideal layout will return fractional values,
    // so you may see slivers between the selection ranges due
    // to the per-primitive antialiasing of the edges unless
    // it is disabled (better for performance anyway).
    target.SetAntialiasing(false);

    for (uint32_t i = 0; i < actualHitTestCount; ++i)
    {
      const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
      D2D1_RECT_F highlightRect          = { htm.left, htm.top, (htm.left + htm.width), (htm.top + htm.height) };

      target.FillRectangle(highlightRect, *textSelectionEffect_.Get());
    }

    target.SetAntialiasing(true);
  }

  // Draw our caret onto the render target.
  MJ_UNINITIALIZED D2D1_RECT_F caretRect;
  this->GetCaretRect(caretRect);
  target.SetAntialiasing(false);
  target.FillRectangle(caretRect, *this->caretBackgroundEffect_.Get());
  target.SetAntialiasing(true);

  // Draw text
  target.DrawTextLayout(this->pTextLayout.Get(), pageRect);

  // Draw the selection ranges in front of images.
  // This shades otherwise opaque images so they are visibly selected,
  // checking the isText field of the hit-test metrics.
  if (actualHitTestCount > 0)
  {
    // Note that an ideal layout will return fractional values,
    // so you may see slivers between the selection ranges due
    // to the per-primitive antialiasing of the edges unless
    // it is disabled (better for performance anyway).
    target.SetAntialiasing(false);

    for (uint32_t i = 0; i < actualHitTestCount; ++i)
    {
      const DWRITE_HIT_TEST_METRICS& htm = hitTestMetrics[i];
      if (htm.isText)
        continue; // Only draw selection if not text.

      D2D1_RECT_F highlightRect = { htm.left, htm.top, (htm.left + htm.width), (htm.top + htm.height) };

      target.FillRectangle(highlightRect, *imageSelectionEffect_.Get());
    }

    target.SetAntialiasing(true);
  }

  // Restore transform
  target.SetTransform(previousTransform);
}

void TextEditor::RefreshView()
{
  // Redraws the text and scrollbars.

  this->UpdateScrollInfo();
  this->PostRedraw();
}

void TextEditor::OnScroll(UINT message, UINT request)
{
  // Reacts to scroll bar changes.

  SCROLLINFO scrollInfo = { sizeof(scrollInfo) };
  scrollInfo.fMask      = SIF_ALL;

  const int32_t barOrientation = (message == WM_VSCROLL) ? SB_VERT : SB_HORZ;

  if (!GetScrollInfo(this->hwnd_, barOrientation, &scrollInfo))
    return;

  // Save the position for comparison later on
  const int32_t oldPosition = scrollInfo.nPos;

  switch (request)
  {
  case SB_TOP:
    scrollInfo.nPos = scrollInfo.nMin;
    break;
  case SB_BOTTOM:
    scrollInfo.nPos = scrollInfo.nMax;
    break;
  case SB_LINEUP:
    scrollInfo.nPos -= 10;
    break;
  case SB_LINEDOWN:
    scrollInfo.nPos += 10;
    break;
  case SB_PAGEUP:
    scrollInfo.nPos -= scrollInfo.nPage;
    break;
  case SB_PAGEDOWN:
    scrollInfo.nPos += scrollInfo.nPage;
    break;
  case SB_THUMBTRACK:
    scrollInfo.nPos = scrollInfo.nTrackPos;
    break;
  default:
    break;
  }

  if (scrollInfo.nPos < 0)
    scrollInfo.nPos = 0;
  if (scrollInfo.nPos > scrollInfo.nMax - static_cast<signed>(scrollInfo.nPage))
    scrollInfo.nPos = scrollInfo.nMax - scrollInfo.nPage;

  scrollInfo.fMask = SIF_POS;
  SetScrollInfo(this->hwnd_, barOrientation, &scrollInfo, TRUE);

  // If the position has changed, scroll the window
  if (scrollInfo.nPos != oldPosition)
  {
    // Need the view matrix in case the editor is flipped/mirrored/rotated.
    D2D1::Matrix3x2F pageTransform;
    GetInverseViewMatrix(&Cast(pageTransform));

    const float inversePos = static_cast<float>(scrollInfo.nMax - scrollInfo.nPage - scrollInfo.nPos);

    D2D1_POINT_2F scaledSize = { pageTransform._11 + pageTransform._21, pageTransform._12 + pageTransform._22 };

    // Adjust the correct origin.
    if ((barOrientation == SB_VERT) ^ IsLandscapeAngle(this->angle_))
    {
      this->originY_ = static_cast<float>(scaledSize.y >= 0 ? scrollInfo.nPos : inversePos);
    }
    else
    {
      this->originX_ = static_cast<float>(scaledSize.x >= 0 ? scrollInfo.nPos : inversePos);
    }

    ConstrainViewOrigin();
    PostRedraw();
  }
}

void TextEditor::UpdateScrollInfo()
{
  // Updates scroll bars.

  if (!this->pTextLayout)
    return;

  // Determine scroll bar's step size in pixels by multiplying client rect by current view.
  RECT clientRect;
  ::GetClientRect(this->hwnd_, &clientRect);

  D2D1::Matrix3x2F pageTransform;
  this->GetInverseViewMatrix(&Cast(pageTransform));

  // Transform vector of viewport size
  D2D1_POINT_2F clientSize = { static_cast<float>(clientRect.right), static_cast<float>(clientRect.bottom) };
  D2D1_POINT_2F scaledSize = { clientSize.x * pageTransform._11 + clientSize.y * pageTransform._21,
                               clientSize.x * pageTransform._12 + clientSize.y * pageTransform._22 };

  float x                = this->originX_;
  float y                = this->originY_;
  D2D1_POINT_2F pageSize = GetPageSize(this->pTextLayout.Get());

  SCROLLINFO scrollInfo = { sizeof(scrollInfo) };
  scrollInfo.fMask      = SIF_PAGE | SIF_POS | SIF_RANGE;

  if (IsLandscapeAngle(this->angle_))
  {
    mj::swap(x, y);
    mj::swap(pageSize.x, pageSize.y);
    mj::swap(scaledSize.x, scaledSize.y);
  }

  // Set vertical scroll bar.
  scrollInfo.nPage = static_cast<int32_t>(mj::abs(scaledSize.y));
  scrollInfo.nPos  = static_cast<int32_t>(scaledSize.y >= 0 ? y : pageSize.y - y);
  scrollInfo.nMin  = 0;
  scrollInfo.nMax  = static_cast<int32_t>(pageSize.y) + scrollInfo.nPage;
  static_cast<void>(SetScrollInfo(this->hwnd_, SB_VERT, &scrollInfo, TRUE));
  scrollInfo.nPos = 0;
  scrollInfo.nMax = 0;
  static_cast<void>(GetScrollInfo(this->hwnd_, SB_VERT, &scrollInfo));

  // Set horizontal scroll bar.
  scrollInfo.nPage = static_cast<int32_t>(mj::abs(scaledSize.x));
  scrollInfo.nPos  = static_cast<int32_t>(scaledSize.x >= 0 ? x : pageSize.x - x);
  scrollInfo.nMin  = 0;
  scrollInfo.nMax  = static_cast<int32_t>(pageSize.x) + scrollInfo.nPage;
  static_cast<void>(SetScrollInfo(this->hwnd_, SB_HORZ, &scrollInfo, TRUE));
}

void TextEditor::OnSize(UINT width, UINT height)
{
  ZoneScoped;

  if (this->renderTarget_)
    this->renderTarget_->Resize(width, height);

  RefreshView();
}

// Input handling.

void TextEditor::OnMousePress(UINT message, float x, float y)
{
  MirrorXCoordinate(x);

  if (message == WM_LBUTTONDOWN)
  {
    // Start dragging selection.
    this->currentlySelecting_ = true;

    const bool heldShift = (GetKeyState(VK_SHIFT) & 0x80) != 0;
    SetSelectionFromPoint(x, y, heldShift);
  }
  else if (message == WM_MBUTTONDOWN)
  {
    this->previousMouseX    = x;
    this->previousMouseY    = y;
    this->currentlyPanning_ = true;
  }
}

void TextEditor::OnMouseRelease(UINT message, float x, float y)
{
  MirrorXCoordinate(x);

  if (message == WM_LBUTTONUP)
  {
    this->currentlySelecting_ = false;
  }
  else if (message == WM_MBUTTONUP)
  {
    this->currentlyPanning_ = false;
  }
}

void TextEditor::OnMouseMove(float x, float y)
{
  // Selects text or pans.

  MirrorXCoordinate(x);

  if (this->currentlySelecting_)
  {
    // Drag current selection.
    SetSelectionFromPoint(x, y, true);
  }
  else if (this->currentlyPanning_)
  {
    DWRITE_MATRIX matrix;
    GetInverseViewMatrix(&matrix);

    const float xDif     = x - this->previousMouseX;
    const float yDif     = y - this->previousMouseY;
    this->previousMouseX = x;
    this->previousMouseY = y;

    this->originX_ -= (xDif * matrix.m11 + yDif * matrix.m21);
    this->originY_ -= (xDif * matrix.m12 + yDif * matrix.m22);
    ConstrainViewOrigin();

    RefreshView();
  }
}

void TextEditor::OnMouseScroll(float xScroll, float yScroll)
{
  // Pans or scales the editor.
  const bool heldShift   = (GetKeyState(VK_SHIFT) & 0x80) != 0;
  const bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;

  if (heldControl)
  {
    // Scale
    const float scaleFactor = (yScroll > 0.0f) ? 1.0625f : 1 / 1.0625f;
    SetScale(scaleFactor, scaleFactor, true);
  }
  else
  {
    // Pan
    DWRITE_MATRIX matrix;
    GetInverseViewMatrix(&matrix);

    yScroll *= MouseScrollFactor;
    xScroll *= MouseScrollFactor; // for mice that support horizontal panning
    if (heldShift)
      mj::swap(xScroll, yScroll);

    this->originX_ -= (xScroll * matrix.m11 + yScroll * matrix.m21);
    this->originY_ -= (xScroll * matrix.m12 + yScroll * matrix.m22);
    ConstrainViewOrigin();

    RefreshView();
  }
}

void TextEditor::OnMouseExit()
{
  this->currentlySelecting_ = false;
  this->currentlyPanning_   = false;
}

void TextEditor::MirrorXCoordinate(IN OUT float& x)
{
  // On RTL builds, coordinates may need to be restored to or converted
  // from Cartesian coordinates, where x increases positively to the right.
  if (GetWindowLong(this->hwnd_, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
  {
    RECT rect;
    GetClientRect(this->hwnd_, &rect);
    x = static_cast<float>(rect.right) - x - 1;
  }
}

void TextEditor::ConstrainViewOrigin()
{
  // Keep the page on-screen by not allowing the origin
  // to go outside the page bounds.

  const D2D1_POINT_2F pageSize = GetPageSize(this->pTextLayout.Get());

  if (this->originX_ > pageSize.x)
    this->originX_ = pageSize.x;
  if (this->originX_ < 0)
    this->originX_ = 0;

  if (this->originY_ > pageSize.y)
    this->originY_ = pageSize.y;
  if (this->originY_ < 0)
    this->originY_ = 0;
}

void TextEditor::OnKeyPress(UINT32 keyCode)
{
  // Handles caret navigation and special presses that
  // do not generate characters.

  const bool heldShift   = (GetKeyState(VK_SHIFT) & 0x80) != 0;
  const bool heldControl = (GetKeyState(VK_CONTROL) & 0x80) != 0;

  const UINT32 absolutePosition = this->caretPosition_ + this->caretPositionOffset_;

  switch (keyCode)
  {
  case VK_RETURN:
    // Insert CR/LF pair
    this->DeleteSelection();
    layoutEditor_.InsertTextAt(this->pTextLayout, this->text_, absolutePosition, L"\r\n", 2, &caretFormat_);
    this->SetSelection(ESetSelectionMode::AbsoluteLeading, absolutePosition + 2, false, false);
    this->RefreshView();
    break;

  case VK_BACK:
    // Erase back one character (less than a character though).
    // Since layout's hit-testing always returns a whole cluster,
    // we do the surrogate pair detection here directly. Otherwise
    // there would be no way to delete just the diacritic following
    // a base character.

    if (absolutePosition != this->caretAnchor_)
    {
      // delete the selected text
      this->DeleteSelection();
    }
    else if (absolutePosition > 0)
    {
      MJ_UNINITIALIZED size_t length;
      // TODO MJ: Handle HRESULT
      ::StringCchLengthW(this->text_.begin(), this->text_.Capacity(), &length);

      UINT32 count = 1;
      // Need special case for surrogate pairs and CR/LF pair.
      if (absolutePosition >= 2 && absolutePosition <= length)
      {
        const wchar_t charBackOne = this->text_[absolutePosition - 1];
        const wchar_t charBackTwo = this->text_[absolutePosition - 2];
        if ((IsLowSurrogate(charBackOne) && IsHighSurrogate(charBackTwo)) ||
            (charBackOne == '\n' && charBackTwo == '\r'))
        {
          count = 2;
        }
      }
      this->SetSelection(ESetSelectionMode::LeftChar, count, false);
      layoutEditor_.RemoveTextAt(this->pTextLayout, this->text_, this->caretPosition_, count);
      this->RefreshView();
    }
    break;

  case VK_DELETE:
    // Delete following cluster.

    if (absolutePosition != this->caretAnchor_)
    {
      // Delete all the selected text.
      DeleteSelection();
    }
    else
    {
      MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
      MJ_UNINITIALIZED FLOAT caretX, caretY;

      // Get the size of the following cluster.
      this->pTextLayout->HitTestTextPosition(absolutePosition, false, &caretX, &caretY, &hitTestMetrics);

      layoutEditor_.RemoveTextAt(this->pTextLayout, this->text_, hitTestMetrics.textPosition, hitTestMetrics.length);

      static_cast<void>(this->SetSelection(ESetSelectionMode::AbsoluteLeading, hitTestMetrics.textPosition, false));
      this->RefreshView();
    }
    break;

  case VK_TAB:
    break; // want tabs

  case VK_LEFT: // seek left one cluster
    this->SetSelection(heldControl ? ESetSelectionMode::LeftWord : ESetSelectionMode::Left, 1, heldShift);
    break;

  case VK_RIGHT: // seek right one cluster
    this->SetSelection(heldControl ? ESetSelectionMode::RightWord : ESetSelectionMode::Right, 1, heldShift);
    break;

  case VK_UP: // up a line
    this->SetSelection(ESetSelectionMode::Up, 1, heldShift);
    break;

  case VK_DOWN: // down a line
    this->SetSelection(ESetSelectionMode::Down, 1, heldShift);
    break;

  case VK_HOME: // beginning of line
    this->SetSelection(heldControl ? ESetSelectionMode::First : ESetSelectionMode::Home, 0, heldShift);
    break;

  case VK_END: // end of line
    this->SetSelection(heldControl ? ESetSelectionMode::Last : ESetSelectionMode::End, 0, heldShift);
    break;

  case 'C':
    if (heldControl)
      this->CopyToClipboard();
    break;

  case VK_INSERT:
    if (heldControl)
      this->CopyToClipboard();
    else if (heldShift)
      this->PasteFromClipboard();
    break;

  case 'V':
    if (heldControl)
      this->PasteFromClipboard();
    break;

  case 'X':
    if (heldControl)
    {
      this->CopyToClipboard();
      this->DeleteSelection();
    }
    break;

  case 'A':
    if (heldControl)
    {
      this->SetSelection(ESetSelectionMode::All, 0, true);
    }
    break;
  }
}

/// <summary>
/// Inserts text characters.
/// </summary>
/// <param name="charCode"></param>
void TextEditor::OnKeyCharacter(UINT32 charCode)
{
  // Allow normal characters and tabs
  if (charCode >= 0x20 || charCode == 9)
  {
    // Replace any existing selection.
    this->DeleteSelection();

    // Convert the UTF32 character code from the Window message to UTF16,
    // yielding 1-2 code-units. Then advance the caret position by how
    // many code-units were inserted.

    UINT32 charsLength = 1;
    wchar_t chars[2]   = { static_cast<wchar_t>(charCode), 0 };

    // If above the basic multi-lingual plane, split into
    // leading and trailing surrogates.
    if (charCode > 0xFFFF)
    {
      // From http://unicode.org/faq/utf_bom.html#35
      chars[0] = static_cast<wchar_t>(0xD800 + (charCode >> 10) - (0x10000 >> 10));
      chars[1] = static_cast<wchar_t>(0xDC00 + (charCode & 0x3FF));
      charsLength++;
    }

    // TODO MJ: Handle HRESULT
    layoutEditor_.InsertTextAt(this->pTextLayout, this->text_, this->caretPosition_ + this->caretPositionOffset_, chars,
                               charsLength, &caretFormat_);
    this->SetSelection(ESetSelectionMode::Right, charsLength, false, false);

    this->RefreshView();
  }
}

// Caret navigation and selection.

UINT32 TextEditor::GetCaretPosition()
{
  return this->caretPosition_ + this->caretPositionOffset_;
}

DWRITE_TEXT_RANGE TextEditor::GetSelectionRange()
{
  // Returns a valid range of the current selection,
  // regardless of whether the caret or anchor is first.

  UINT32 caretBegin = this->caretAnchor_;
  UINT32 caretEnd   = this->caretPosition_ + this->caretPositionOffset_;
  if (caretBegin > caretEnd)
    mj::swap(caretBegin, caretEnd);

  MJ_UNINITIALIZED size_t length;
  // TODO MJ: Handle HRESULT
  ::StringCchLengthW(this->text_.begin(), this->text_.Capacity(), &length);

  // Limit to actual text length.
  const UINT32 textLength = static_cast<UINT32>(length);
  caretBegin              = mj_min(caretBegin, textLength);
  caretEnd                = mj_min(caretEnd, textLength);

  return DWRITE_TEXT_RANGE{ caretBegin, caretEnd - caretBegin };
}

void TextEditor::GetLineMetrics(OUT mj::ArrayList<DWRITE_LINE_METRICS>& lineMetrics)
{
  // Retrieves the line metrics, used for caret navigation, up/down and home/end.

  MJ_UNINITIALIZED DWRITE_TEXT_METRICS textMetrics;
  // TODO MJ: Handle HRESULT
  this->pTextLayout->GetMetrics(&textMetrics);

  // TODO MJ: Handle nullptr
  lineMetrics.Reserve(textMetrics.lineCount);

  // TODO MJ: Handle HRESULT
  this->pTextLayout->GetLineMetrics(&lineMetrics[0], textMetrics.lineCount, &textMetrics.lineCount);
}

void TextEditor::GetLineFromPosition(const DWRITE_LINE_METRICS* lineMetrics, // [lineCount]
                                     UINT32 lineCount, UINT32 textPosition, OUT UINT32* lineOut,
                                     OUT UINT32* linePositionOut)
{
  // Given the line metrics, determines the current line and starting text
  // position of that line by summing up the lengths. When the starting
  // line position is beyond the given text position, we have our line.

  UINT32 line             = 0;
  UINT32 linePosition     = 0;
  UINT32 nextLinePosition = 0;

  for (; line < lineCount; ++line)
  {
    linePosition     = nextLinePosition;
    nextLinePosition = linePosition + lineMetrics[line].length;
    if (nextLinePosition > textPosition)
    {
      // The next line is beyond the desired text position,
      // so it must be in the current line.
      break;
    }
  }

  *linePositionOut = linePosition;
  *lineOut         = mj_min(line, lineCount - 1);
}

void TextEditor::AlignCaretToNearestCluster(bool isTrailingHit, bool skipZeroWidth)
{
  // Uses hit-testing to align the current caret position to a whole cluster,
  // rather than residing in the middle of a base character + diacritic,
  // surrogate pair, or character + UVS.

  MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
  MJ_UNINITIALIZED float caretX, caretY;

  // Align the caret to the nearest whole cluster.
  this->pTextLayout->HitTestTextPosition(this->caretPosition_, false, &caretX, &caretY, &hitTestMetrics);

  // The caret position itself is always the leading edge.
  // An additional offset indicates a trailing edge when non-zero.
  // This offset comes from the number of code-units in the
  // selected cluster or surrogate pair.
  this->caretPosition_       = hitTestMetrics.textPosition;
  this->caretPositionOffset_ = (isTrailingHit) ? hitTestMetrics.length : 0;

  // For invisible, zero-width characters (like line breaks
  // and formatting characters), force leading edge of the
  // next position.
  if (skipZeroWidth && hitTestMetrics.width == 0)
  {
    this->caretPosition_ += this->caretPositionOffset_;
    this->caretPositionOffset_ = 0;
  }
}

bool TextEditor::SetSelectionFromPoint(float x, float y, bool extendSelection)
{
  // Returns the text position corresponding to the mouse x,y.
  // If hitting the trailing side of a cluster, return the
  // leading edge of the following text position.

  MJ_UNINITIALIZED BOOL isTrailingHit;
  MJ_UNINITIALIZED BOOL isInside;
  MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS caretMetrics;

  // Remap display coordinates to actual.
  MJ_UNINITIALIZED DWRITE_MATRIX matrix;
  this->GetInverseViewMatrix(&matrix);

  const float transformedX = (x * matrix.m11 + y * matrix.m21 + matrix.dx);
  const float transformedY = (x * matrix.m12 + y * matrix.m22 + matrix.dy);

  this->pTextLayout->HitTestPoint(transformedX, transformedY, &isTrailingHit, &isInside, &caretMetrics);

  // Update current selection according to click or mouse drag.
  SetSelection(isTrailingHit ? ESetSelectionMode::AbsoluteTrailing : ESetSelectionMode::AbsoluteLeading,
               caretMetrics.textPosition, extendSelection);

  return true;
}

/// <summary>
/// Moves the caret relatively or absolutely, optionally extending the
/// selection range (for example, when shift is held).
/// </summary>
/// <param name="moveMode"></param>
/// <param name="advance"></param>
/// <param name="extendSelection"></param>
/// <param name="updateCaretFormat"></param>
/// <returns>True if the caret was moved</returns>
bool TextEditor::SetSelection(ESetSelectionMode::Enum moveMode, UINT32 advance, bool extendSelection,
                              bool updateCaretFormat)
{

  UINT32 line                      = UINT32_MAX; // current line number, needed by a few modes
  UINT32 absolutePosition          = this->caretPosition_ + this->caretPositionOffset_;
  const UINT32 oldAbsolutePosition = absolutePosition;
  const UINT32 oldCaretAnchor      = this->caretAnchor_;

  MJ_UNINITIALIZED size_t length;
  // TODO MJ: Handle HRESULT
  ::StringCchLengthW(this->text_.begin(), this->text_.Capacity(), &length);

  switch (moveMode)
  {
  case ESetSelectionMode::Left:
    this->caretPosition_ += this->caretPositionOffset_;
    if (this->caretPosition_ > 0)
    {
      --this->caretPosition_;
      this->AlignCaretToNearestCluster(false, true);

      // special check for CR/LF pair
      absolutePosition = this->caretPosition_ + this->caretPositionOffset_;
      if (absolutePosition >= 1 && absolutePosition < length && this->text_[absolutePosition - 1] == '\r' &&
          this->text_[absolutePosition] == '\n')
      {
        this->caretPosition_ = absolutePosition - 1;
        this->AlignCaretToNearestCluster(false, true);
      }
    }
    break;

  case ESetSelectionMode::Right:
    this->caretPosition_ = absolutePosition;
    this->AlignCaretToNearestCluster(true, true);

    // special check for CR/LF pair
    absolutePosition = this->caretPosition_ + this->caretPositionOffset_;
    if (absolutePosition >= 1 && absolutePosition < length && this->text_[absolutePosition - 1] == '\r' &&
        this->text_[absolutePosition] == '\n')
    {
      this->caretPosition_ = absolutePosition + 1;
      this->AlignCaretToNearestCluster(false, true);
    }
    break;

  case ESetSelectionMode::LeftChar:
    this->caretPosition_ = absolutePosition;
    this->caretPosition_ -= mj_min(advance, absolutePosition);
    this->caretPositionOffset_ = 0;
    break;

  case ESetSelectionMode::RightChar:
    this->caretPosition_       = absolutePosition + advance;
    this->caretPositionOffset_ = 0;
    {
      // Use hit-testing to limit text position.
      MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
      MJ_UNINITIALIZED float caretX, caretY;

      this->pTextLayout->HitTestTextPosition(this->caretPosition_, false, &caretX, &caretY, &hitTestMetrics);
      this->caretPosition_ = mj_min(this->caretPosition_, hitTestMetrics.textPosition + hitTestMetrics.length);
    }
    break;

  case ESetSelectionMode::Up:
  case ESetSelectionMode::Down:
  {
    // Retrieve the line metrics to figure out what line we are on.
    mj::Allocator allocator(nullptr, mj::Win32Alloc, mj::Win32Free);
    mj::ArrayList<DWRITE_LINE_METRICS> lineMetrics(&allocator);
    GetLineMetrics(lineMetrics);

    MJ_UNINITIALIZED UINT32 linePosition;
    GetLineFromPosition(&lineMetrics[0], static_cast<UINT32>(lineMetrics.Size()), this->caretPosition_, &line,
                        &linePosition);

    // Move up a line or down
    if (moveMode == ESetSelectionMode::Up)
    {
      if (line <= 0)
        break; // already top line
      line--;
      linePosition -= lineMetrics[line].length;
    }
    else
    {
      linePosition += lineMetrics[line].length;
      line++;
      if (line >= lineMetrics.Size())
        break; // already bottom line
    }

    // To move up or down, we need three hit-testing calls to determine:
    // 1. The x of where we currently are.
    // 2. The y of the new line.
    // 3. New text position from the determined x and y.
    // This is because the characters are variable size.

    MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
    MJ_UNINITIALIZED float caretX, caretY, dummyX;

    // Get x of current text position
    this->pTextLayout->HitTestTextPosition(this->caretPosition_,
                                           this->caretPositionOffset_ > 0, // trailing if nonzero, else leading edge
                                           &caretX, &caretY, &hitTestMetrics);

    // Get y of new position
    this->pTextLayout->HitTestTextPosition(linePosition,
                                           false, // leading edge
                                           &dummyX, &caretY, &hitTestMetrics);

    // Now get text position of new x,y.
    BOOL isInside, isTrailingHit;
    this->pTextLayout->HitTestPoint(caretX, caretY, &isTrailingHit, &isInside, &hitTestMetrics);

    this->caretPosition_       = hitTestMetrics.textPosition;
    this->caretPositionOffset_ = isTrailingHit ? (hitTestMetrics.length > 0) : 0;
  }
  break;

  case ESetSelectionMode::LeftWord:
  case ESetSelectionMode::RightWord:
  {
    // To navigate by whole words, we look for the canWrapLineAfter
    // flag in the cluster metrics.

    // First need to know how many clusters there are.
    mj::Allocator allocator(nullptr, mj::Win32Alloc, mj::Win32Free);
    mj::ArrayList<DWRITE_CLUSTER_METRICS> clusterMetrics(&allocator);
    MJ_UNINITIALIZED UINT32 clusterCount;
    this->pTextLayout->GetClusterMetrics(nullptr, 0, &clusterCount);

    if (clusterCount == 0)
      break;

    // Now we actually read them.
    clusterMetrics.Reserve(clusterCount);
    this->pTextLayout->GetClusterMetrics(&clusterMetrics[0], clusterCount, &clusterCount);

    this->caretPosition_ = absolutePosition;

    UINT32 clusterPosition        = 0;
    const UINT32 oldCaretPosition = this->caretPosition_;

    if (moveMode == ESetSelectionMode::LeftWord)
    {
      // Read through the clusters, keeping track of the farthest valid
      // stopping point just before the old position.
      this->caretPosition_       = 0;
      this->caretPositionOffset_ = 0; // leading edge
      for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
      {
        clusterPosition += clusterMetrics[cluster].length;
        if (clusterMetrics[cluster].canWrapLineAfter)
        {
          if (clusterPosition >= oldCaretPosition)
            break;

          // Update in case we pass this point next loop.
          this->caretPosition_ = clusterPosition;
        }
      }
    }
    else // ESetSelectionMode::RightWord
    {
      // Read through the clusters, looking for the first stopping point
      // after the old position.
      for (UINT32 cluster = 0; cluster < clusterCount; ++cluster)
      {
        const UINT32 clusterLength = clusterMetrics[cluster].length;
        this->caretPosition_       = clusterPosition;
        this->caretPositionOffset_ = clusterLength; // trailing edge
        if (clusterPosition >= oldCaretPosition && clusterMetrics[cluster].canWrapLineAfter)
          break; // first stopping point after old position.

        clusterPosition += clusterLength;
      }
    }
  }
  break;

  case ESetSelectionMode::Home:
  case ESetSelectionMode::End:
  {
    // Retrieve the line metrics to know first and last position
    // on the current line.
    mj::Allocator allocator(nullptr, mj::Win32Alloc, mj::Win32Free);
    mj::ArrayList<DWRITE_LINE_METRICS> lineMetrics(&allocator);
    GetLineMetrics(lineMetrics);

    GetLineFromPosition(&lineMetrics[0], static_cast<UINT32>(lineMetrics.Size()), this->caretPosition_, &line,
                        &this->caretPosition_);

    this->caretPositionOffset_ = 0;
    if (moveMode == ESetSelectionMode::End)
    {
      // Place the caret at the last character on the line,
      // excluding line breaks. In the case of wrapped lines,
      // newlineLength will be 0.
      const UINT32 lineLength    = lineMetrics[line].length - lineMetrics[line].newlineLength;
      this->caretPositionOffset_ = mj_min(lineLength, 1u);
      this->caretPosition_ += lineLength - this->caretPositionOffset_;
      AlignCaretToNearestCluster(true);
    }
  }
  break;

  case ESetSelectionMode::First:
    this->caretPosition_       = 0;
    this->caretPositionOffset_ = 0;
    break;

  case ESetSelectionMode::All:
    this->caretAnchor_ = 0;
    extendSelection    = true;

  case ESetSelectionMode::Last: // Fall-through
    this->caretPosition_       = UINT32_MAX;
    this->caretPositionOffset_ = 0;
    AlignCaretToNearestCluster(true);
    break;

  case ESetSelectionMode::AbsoluteLeading:
    this->caretPosition_       = advance;
    this->caretPositionOffset_ = 0;
    break;

  case ESetSelectionMode::AbsoluteTrailing:
    this->caretPosition_ = advance;
    AlignCaretToNearestCluster(true);
    break;
  }

  absolutePosition = this->caretPosition_ + this->caretPositionOffset_;

  if (!extendSelection)
    this->caretAnchor_ = absolutePosition;

  const bool caretMoved = (absolutePosition != oldAbsolutePosition) || (this->caretAnchor_ != oldCaretAnchor);

  if (caretMoved)
  {
    // update the caret formatting attributes
    if (updateCaretFormat)
      UpdateCaretFormatting();

    PostRedraw();

    D2D1_RECT_F rect;
    GetCaretRect(rect);
    UpdateSystemCaret(rect);
  }

  return caretMoved;
}

void TextEditor::GetCaretRect(OUT D2D1_RECT_F& rect)
{
  // Gets the current caret position (in untransformed space).

  D2D1_RECT_F zeroRect = {};
  rect                 = zeroRect;

  if (!this->pTextLayout)
    return;

  // Translate text character offset to point x,y.
  DWRITE_HIT_TEST_METRICS caretMetrics;
  float caretX, caretY;

  this->pTextLayout->HitTestTextPosition(this->caretPosition_,
                                         this->caretPositionOffset_ > 0, // trailing if nonzero, else leading edge
                                         &caretX, &caretY, &caretMetrics);

  // If a selection exists, draw the caret using the
  // line size rather than the font size.
  const DWRITE_TEXT_RANGE selectionRange = GetSelectionRange();
  if (selectionRange.length > 0)
  {
    UINT32 actualHitTestCount = 1;
    this->pTextLayout->HitTestTextRange(this->caretPosition_,
                                        0, // length
                                        0, // x
                                        0, // y
                                        &caretMetrics, 1, &actualHitTestCount);

    caretY = caretMetrics.top;
  }

  // The default thickness of 1 pixel is almost _too_ thin on modern large monitors,
  // but we'll use it.
  DWORD caretIntThickness = 2;
  SystemParametersInfoW(SPI_GETCARETWIDTH, 0, &caretIntThickness, FALSE);
  const float caretThickness = static_cast<float>(caretIntThickness);

  // Return the caret rect, untransformed.
  rect.left   = caretX - caretThickness / 2.0f;
  rect.right  = rect.left + caretThickness;
  rect.top    = caretY;
  rect.bottom = caretY + caretMetrics.height;
}

void TextEditor::UpdateSystemCaret(const D2D1_RECT_F& rect)
{
  // Moves the system caret to a new position.

  // Although we don't actually use the system caret (drawing our own
  // instead), this is important for accessibility, so the magnifier
  // can follow text we type. The reason we draw our own directly
  // is because intermixing DirectX and GDI content (the caret) reduces
  // performance.

  // Gets the current caret position (in untransformed space).

  if (GetFocus() != this->hwnd_) // Only update if we have focus.
    return;

  D2D1::Matrix3x2F pageTransform;
  GetViewMatrix(&Cast(pageTransform));

  // Transform caret top/left and size according to current scale and origin.
  const D2D1_POINT_2F caretPoint = pageTransform.TransformPoint(D2D1::Point2F(rect.left, rect.top));

  const float width             = (rect.right - rect.left);
  const float height            = (rect.bottom - rect.top);
  const float transformedWidth  = width * pageTransform._11 + height * pageTransform._21;
  const float transformedHeight = width * pageTransform._12 + height * pageTransform._22;

  // Update the caret's location, rounding to nearest integer so that
  // it lines up with the text selection.

  const int32_t intX      = RoundToInt(caretPoint.x);
  const int32_t intY      = RoundToInt(caretPoint.y);
  const int32_t intWidth  = RoundToInt(transformedWidth);
  const int32_t intHeight = RoundToInt(caretPoint.y + transformedHeight) - intY;

  CreateCaret(this->hwnd_, nullptr, intWidth, intHeight);
  SetCaretPos(intX, intY);

  // Don't actually call ShowCaret. It's enough to just set its position.
}

void TextEditor::UpdateCaretFormatting()
{
  UINT32 currentPos = this->caretPosition_ + this->caretPositionOffset_;

  if (currentPos > 0)
  {
    --currentPos; // Always adopt the trailing properties.
  }

  // Get the family name
  caretFormat_.fontFamilyName[0] = '\0';
  this->pTextLayout->GetFontFamilyName(currentPos, &caretFormat_.fontFamilyName[0],
                                       ARRAYSIZE(caretFormat_.fontFamilyName));

  // Get the locale
  caretFormat_.localeName[0] = '\0';
  this->pTextLayout->GetLocaleName(currentPos, &caretFormat_.localeName[0], ARRAYSIZE(caretFormat_.localeName));

  // Get the remaining attributes...
  this->pTextLayout->GetFontWeight(currentPos, &caretFormat_.fontWeight);
  this->pTextLayout->GetFontStyle(currentPos, &caretFormat_.fontStyle);
  this->pTextLayout->GetFontStretch(currentPos, &caretFormat_.fontStretch);
  this->pTextLayout->GetFontSize(currentPos, &caretFormat_.fontSize);
  this->pTextLayout->GetUnderline(currentPos, &caretFormat_.hasUnderline);
  this->pTextLayout->GetStrikethrough(currentPos, &caretFormat_.hasStrikethrough);

  // Get the current color.
  mj::ComPtr<IUnknown> drawingEffect;
  this->pTextLayout->GetDrawingEffect(currentPos, drawingEffect.GetAddressOf());
  caretFormat_.color = 0;
  if (drawingEffect)
  {
    const DrawingEffect& effect = *reinterpret_cast<DrawingEffect*>(drawingEffect.Get());
    caretFormat_.color          = effect.GetColor();
  }
}

// Selection/clipboard actions.

void TextEditor::CopyToClipboard()
{
  // Copies selected text to clipboard.

  const DWRITE_TEXT_RANGE selectionRange = GetSelectionRange();
  if (selectionRange.length <= 0)
    return;

  // Open and empty existing contents.
  if (OpenClipboard(this->hwnd_))
  {
    if (EmptyClipboard())
    {
      // Allocate room for the text
      const size_t byteSize  = sizeof(wchar_t) * (static_cast<size_t>(selectionRange.length) + 1);
      HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE | GMEM_ZEROINIT, byteSize);

      if (hClipboardData)
      {
        void* memory = GlobalLock(hClipboardData); // [byteSize] in bytes

        if (memory)
        {
          // Copy text to memory block.
          const wchar_t* text = this->text_.begin();
          memcpy(memory, &text[selectionRange.startPosition], byteSize);
          GlobalUnlock(hClipboardData);

          if (SetClipboardData(CF_UNICODETEXT, hClipboardData))
          {
            hClipboardData = nullptr; // system now owns the clipboard, so don't touch it.
          }
        }
        GlobalFree(hClipboardData); // free if failed
      }
    }
    CloseClipboard();
  }
}

/// <summary>
/// Deletes selection.
/// </summary>
void TextEditor::DeleteSelection()
{
  const DWRITE_TEXT_RANGE selectionRange = GetSelectionRange();
  if (selectionRange.length <= 0)
  {
    return;
  }

  // TODO MJ: Handle HRESULT
  layoutEditor_.RemoveTextAt(this->pTextLayout, this->text_, selectionRange.startPosition, selectionRange.length);

  static_cast<void>(this->SetSelection(ESetSelectionMode::AbsoluteLeading, selectionRange.startPosition, false));
  this->RefreshView();
}

/// <summary>
/// Pastes text from clipboard at current caret position.
/// </summary>
void TextEditor::PasteFromClipboard()
{
  this->DeleteSelection();

  UINT32 characterCount = 0;

  // Copy Unicode text from clipboard.

  if (::OpenClipboard(this->hwnd_))
  {
    HGLOBAL hClipboardData = GetClipboardData(CF_UNICODETEXT);

    if (hClipboardData)
    {
      // Get text and size of text.
      // size_t byteSize     = GlobalSize(hClipboardData);
      void* pMemory       = GlobalLock(hClipboardData); // [byteSize] in bytes
      const wchar_t* text = reinterpret_cast<const wchar_t*>(pMemory);
      MJ_UNINITIALIZED size_t length;
      // TODO MJ: Handle HRESULT
      ::StringCchLengthW(text, 1024, &length); // TODO MJ: Fixed buffer size
      characterCount = static_cast<UINT32>(length);

      if (pMemory)
      {
        // Insert the text at the current position.
        layoutEditor_.InsertTextAt(this->pTextLayout, this->text_, this->caretPosition_ + this->caretPositionOffset_,
                                   text, characterCount);
        GlobalUnlock(hClipboardData);
      }
    }
    // TODO MJ: If the function fails, the return value is zero. To get extended error information, call GetLastError.
    ::CloseClipboard();
  }

  static_cast<void>(this->SetSelection(ESetSelectionMode::RightChar, characterCount, true));
  this->RefreshView();
}

HRESULT TextEditor::InsertText(const wchar_t* text)
{
  const UINT32 absolutePosition = this->caretPosition_ + this->caretPositionOffset_;

  MJ_UNINITIALIZED size_t length;
  // TODO MJ: Handle HRESULT
  ::StringCchLengthW(text, 1024, &length); // TODO MJ: Fixed buffer size

  return layoutEditor_.InsertTextAt(this->pTextLayout, this->text_, absolutePosition, text, static_cast<UINT32>(length),
                                    &caretFormat_);
}

HRESULT TextEditor::SetText(const wchar_t* text)
{
  // TODO MJ: Handle HRESULT
  this->layoutEditor_.Clear(this->pTextLayout, this->text_);
  return this->InsertText(text);
}

// Current view.

void TextEditor::ResetView()
{
  // Resets the default view.

  InitViewDefaults();

  // Center document
  const float layoutWidth  = this->pTextLayout->GetMaxWidth();
  const float layoutHeight = this->pTextLayout->GetMaxHeight();
  this->originX_           = layoutWidth / 2;
  this->originY_           = layoutHeight / 2;

  RefreshView();
}

float TextEditor::SetAngle(float angle, bool relativeAdjustement)
{
  if (relativeAdjustement)
    this->angle_ += angle;
  else
    this->angle_ = angle;

  RefreshView();

  return this->angle_;
}

void TextEditor::SetScale(float scaleX, float scaleY, bool relativeAdjustement)
{
  if (relativeAdjustement)
  {
    this->scaleX_ *= scaleX;
    this->scaleY_ *= scaleY;
  }
  else
  {
    this->scaleX_ = scaleX;
    this->scaleY_ = scaleY;
  }
  RefreshView();
}

void TextEditor::GetScale(OUT float* scaleX, OUT float* scaleY)
{
  *scaleX = this->scaleX_;
  *scaleY = this->scaleY_;
}

void TextEditor::GetViewMatrix(OUT DWRITE_MATRIX* matrix) const
{
  // Generates a view matrix from the current origin, angle, and scale.

  // Need the editor size for centering.
  MJ_UNINITIALIZED RECT rect;
  GetClientRect(this->hwnd_, &rect);

  // Translate the origin to 0,0
  DWRITE_MATRIX translationMatrix = { 1.0f,
                                      0.0f, //
                                      0.0f,
                                      1.0f, //
                                      -this->originX_,
                                      -this->originY_ };

  // Scale and rotate
  const float radians = DegreesToRadians(mj::fmodf(this->angle_, 360.0f));
  float cosValue      = mj::cos(radians);
  float sinValue      = mj::sin(radians);

  // If rotation is a quarter multiple, ensure sin and cos are exactly one of {-1,0,1}
  if (mj::fmodf(this->angle_, 90.0f) == 0.0f)
  {
    cosValue = mj::floorf(cosValue + 0.5f);
    sinValue = mj::floorf(sinValue + 0.5f);
  }

  DWRITE_MATRIX rotationMatrix = {
    cosValue * this->scaleX_, sinValue * this->scaleX_, -sinValue * this->scaleY_, cosValue * this->scaleY_, 0.0f, 0.0f
  };

  // Set the origin in the center of the window
  const float centeringFactor = 0.5f;
  DWRITE_MATRIX centerMatrix  = { 1.0f,
                                 0.0f,
                                 0.0f,
                                 1.0f,
                                 mj::floorf(static_cast<float>(rect.right) * centeringFactor),
                                 mj::floorf(static_cast<float>(rect.bottom) * centeringFactor) };

  MJ_UNINITIALIZED D2D1::Matrix3x2F resultA, resultB;

  resultB.SetProduct(Cast(translationMatrix), Cast(rotationMatrix));
  resultA.SetProduct(resultB, Cast(centerMatrix));

  // For better pixel alignment (less blurry text)
  resultA._31 = mj::floorf(resultA._31);
  resultA._32 = mj::floorf(resultA._32);

  *matrix = *reinterpret_cast<DWRITE_MATRIX*>(&resultA);
}

void TextEditor::GetInverseViewMatrix(OUT DWRITE_MATRIX* matrix) const
{
  // Inverts the view matrix for hit-testing and scrolling.
  MJ_UNINITIALIZED DWRITE_MATRIX viewMatrix;
  this->GetViewMatrix(&viewMatrix);
  ComputeInverseMatrix(viewMatrix, *matrix);
}
