#include "mj_textedit.h"
#include <d2d1.h>
#include "mj_common.h"
#include "mj_win32.h"
#include "loremipsum.h"
#include "stretchy_buffer.h"
#include "render_target_resources.h"
#include <Strsafe.h>

static constexpr size_t BUFFER_SIZE   = 2 * 1024 * 1024; // 2 MiB
static constexpr FLOAT SCROLLBAR_SIZE = 20.0f;

HRESULT mj::TextEdit::Init(FLOAT left, FLOAT top, FLOAT right, FLOAT bottom)
{
  this->widgetRect.left   = left;
  this->widgetRect.top    = top;
  this->widgetRect.right  = right;
  this->widgetRect.bottom = bottom;

  HRESULT hr = S_OK;
  // Init memory for buffer
  this->pMemory = VirtualAlloc(0, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!this->pMemory)
  {
    hr = E_FAIL;
  }

  this->buf.Init(this->pMemory, ((char*)this->pMemory) + BUFFER_SIZE);
  this->buf.SetText(pLoremIpsum);

  this->horizontalScrollBar.Init(this);

  this->line.pText       = nullptr;
  this->line.textLength  = 0;
  this->line.pTextLayout = nullptr;

  return hr;
}

void mj::TextEdit::Destroy()
{
  if (this->pMemory)
  {
    MJ_DISCARD(VirtualFree(this->pMemory, 0, MEM_RELEASE));
    this->pMemory = nullptr;
  }
}

void mj::TextEdit::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  (void)lParam;
  (void)hwnd;
  switch (message)
  {
  case WM_CHAR:
    switch (wParam)
    {
    case 0x08: // Backspace
      this->buf.BackspaceAtCaret();
      break;
    case 0x0A: // Line feed
      break;
    case 0x1B: // Escape
      break;
    default:
      this->buf.InsertCharacterAtCaret((wchar_t)wParam);
      break;
    }
    break;
  case WM_KEYDOWN: // Navigation
    switch (wParam)
    {
    case VK_HOME:
      MJ_DISCARD(this->buf.JumpStartOfLine());
      break;
    case VK_END:
      MJ_DISCARD(this->buf.JumpEndOfLine());
      break;
    case VK_LEFT:
      MJ_DISCARD(this->buf.DecrementCaret());
      break;
    case VK_RIGHT:
      MJ_DISCARD(this->buf.IncrementCaret());
      break;
    case VK_UP:
      MJ_DISCARD(this->buf.CaretLinePrev());
      break;
    case VK_DOWN:
      MJ_DISCARD(this->buf.CaretLineNext());
      break;
    case VK_DELETE:
      this->buf.DeleteAtCaret();
      break;
    }
    break;
  }
}

static D2D1_POINT_2F operator*(const D2D1_MATRIX_3X2_F& matrix, const D2D1_POINT_2F& point)
{
  return D2D1::Matrix3x2F::ReinterpretBaseType(&matrix)->TransformPoint(point);
}

void mj::HorizontalScrollBar::Init(TextEdit* pParent)
{
  this->pParent          = pParent;
  this->horScrollbarRect = {};
}

void mj::HorizontalScrollBar::Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  D2D1_RECT_F widgetRect = this->pParent->GetWidgetRect();
  FLOAT scrollPos        = this->pParent->GetScrollPosition().x;

  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F newTransform =
      transform * D2D1::Matrix3x2F::Translation(0.0f, widgetRect.bottom - widgetRect.top - SCROLLBAR_SIZE);
  pRenderTarget->SetTransform(MJ_REF newTransform);

  const auto widgetWidth = (widgetRect.right - widgetRect.left);

  // TODO: fix for high DPI
  MJ_UNINITIALIZED D2D1_RECT_F rect;
  rect.left   = 0.0f;
  rect.top    = 0.0f;
  rect.right  = widgetWidth;
  rect.bottom = SCROLLBAR_SIZE;
  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBackgroundBrush);

  rect.left  = scrollPos / this->pParent->GetWidth() * widgetWidth;
  rect.right = (scrollPos + widgetWidth) / this->pParent->GetWidth() * widgetWidth;

  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBrush);

  // Save for reverse lookup
  auto topLeft           = newTransform * D2D1_POINT_2F{ rect.left, rect.top };
  auto bottomRight       = newTransform * D2D1_POINT_2F{ rect.right, rect.bottom };
  this->horScrollbarRect = RECT{ (LONG)topLeft.x, (LONG)topLeft.y, (LONG)bottomRight.x, (LONG)bottomRight.y };

  pRenderTarget->SetTransform(MJ_REF transform);
}

bool mj::HorizontalScrollBar::MouseDown(SHORT x, SHORT y)
{
  POINT pt{ (LONG)x, (LONG)y };
  return PtInRect(&this->horScrollbarRect, pt);
}

void mj::TextEdit::DrawCaret(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  // Caret
  if (this->line.pTextLayout)
  {
    DWRITE_HIT_TEST_METRICS hitTestMetrics;
    float caretX, caretY;
    bool isTrailingHit = false; // Use the leading character edge for simplicity here.

    // Map text position index to caret coordinate and hit-test rectangle.
    MJ_DISCARD(this->line.pTextLayout->HitTestTextPosition(this->buf.GetVirtualCaretPosition(), isTrailingHit, &caretX,
                                                           &caretY, &hitTestMetrics));
    // Respect user settings.
    DWORD caretWidth = 1;
    SystemParametersInfo(SPI_GETCARETWIDTH, 0, &caretWidth, 0);
    DWORD halfCaretWidth = caretWidth / 2u;

    // Draw a thin rectangle.
    MJ_UNINITIALIZED D2D1_RECT_F rect;
    rect.left   = caretX - halfCaretWidth;
    rect.top    = hitTestMetrics.top;
    rect.right  = caretX + (caretWidth - halfCaretWidth);
    rect.bottom = hitTestMetrics.top + hitTestMetrics.height;

    pRenderTarget->FillRectangle(&rect, pResources->pCaretBrush);
  }
}

void mj::TextEdit::Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  // Background
  pRenderTarget->FillRectangle(MJ_REF this->widgetRect, pResources->pTextEditBackgroundBrush);
  pRenderTarget->PushAxisAlignedClip(MJ_REF this->widgetRect, D2D1_ANTIALIAS_MODE_ALIASED);
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F xBackGround;
  pRenderTarget->GetTransform(&xBackGround);

  pRenderTarget->SetTransform(xBackGround * D2D1::Matrix3x2F::Translation(this->widgetRect.left, this->widgetRect.top));
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F xWidget;
  pRenderTarget->GetTransform(&xWidget);

  pRenderTarget->SetTransform(xWidget * D2D1::Matrix3x2F::Translation(-this->scrollPos.x, -this->scrollPos.y));
  pRenderTarget->DrawTextLayout({}, this->line.pTextLayout, pResources->pTextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);
  this->DrawCaret(pRenderTarget, pResources);

  pRenderTarget->SetTransform(MJ_REF xWidget);
  this->horizontalScrollBar.Draw(pRenderTarget, pResources);

  pRenderTarget->PopAxisAlignedClip();
  pRenderTarget->SetTransform(MJ_REF xBackGround);
}

static bool RectContainsPoint(D2D1_RECT_F* pRect, D2D1_POINT_2F* pPoint)
{
  return ((pPoint->x >= pRect->left) && (pPoint->x <= pRect->right) && (pPoint->y >= pRect->top) &&
          (pPoint->y < (pRect->bottom)));
}

void mj::TextEdit::MouseDown(SHORT x, SHORT y)
{
  // Scroll bar
  if (this->horizontalScrollBar.MouseDown(x, y))
  {
    // Use left edge of scroll bar
    this->drag.start       = this->scrollPos.x;
    this->drag.mouseStartX = x;
    // this->drag.mouseStartY = y;
    this->drag.draggable = EDraggable::HOR_SCROLLBAR;
  }
  else
  {
    this->drag.draggable = EDraggable::NONE;
  }

  // Caret
  if (this->line.pTextLayout)
  {
    MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
    MJ_UNINITIALIZED BOOL isTrailingHit;
    MJ_UNINITIALIZED BOOL isInside;

    MJ_DISCARD(
        this->line.pTextLayout->HitTestPoint(((FLOAT)x), ((FLOAT)y), &isTrailingHit, &isInside, &hitTestMetrics));

    if (isInside)
    {
      this->buf.SetCaretPosition(hitTestMetrics.textPosition);
    }
  }
}

void mj::TextEdit::MouseUp()
{
  this->drag.draggable = mj::EDraggable::NONE;
}

mj::ECursor::Enum mj::TextEdit::MouseMove(SHORT x, SHORT y)
{
  // Check dragging
  switch (this->drag.draggable)
  {
  case EDraggable::HOR_SCROLLBAR:
  {
    const auto widgetWidth = (this->widgetRect.right - this->widgetRect.left);
    SHORT dx               = x - this->drag.mouseStartX;
    this->scrollPos.x      = this->drag.start + (dx / widgetWidth * this->width);
    if (this->scrollPos.x < 0.0f)
    {
      this->scrollPos.x = 0.0f;
    }
    else if ((this->scrollPos.x + widgetWidth) > this->width)
    {
      this->scrollPos.x = (this->width - widgetWidth);
    }
  }
  break;
  default:
    break;
  }

  MJ_UNINITIALIZED D2D1_POINT_2F p;
  p.x       = (FLOAT)x;
  p.y       = (FLOAT)y;
  auto rect = this->widgetRect;
  rect.bottom -= SCROLLBAR_SIZE;
  if (RectContainsPoint(&rect, &p))
  {
    return mj::ECursor::IBEAM;
  }

  return mj::ECursor::ARROW;
}

HRESULT mj::TextEdit::CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat, FLOAT width,
                                            FLOAT height)
{
  // Set longest line width equal to widget width
  this->width = (this->widgetRect.right - this->widgetRect.left);

  // Convert UTF-8 from TextEdit to Win32 wide string
  int numWideCharsLeft  = this->buf.GetLeftLength();
  int numWideCharsRight = this->buf.GetRightLength();
  int numWideCharsTotal = numWideCharsLeft + numWideCharsRight;

  // Delete all rendered lines
  if (this->line.pTextLayout)
  {
    this->line.pTextLayout->Release();
  }
  delete[] this->line.pText;

  wchar_t* pText = new wchar_t[numWideCharsTotal];
  if (pText)
  {
    CopyMemory(pText, this->buf.GetLeftPtr(), numWideCharsLeft * sizeof(wchar_t));
    CopyMemory(pText + numWideCharsLeft, this->buf.GetRightPtr(), numWideCharsRight * sizeof(wchar_t));
  }

  this->line.pText      = pText;
  this->line.textLength = numWideCharsTotal;

  HRESULT hr = pFactory->CreateTextLayout(
      this->line.pText,                // The string to be laid out and formatted.
      (UINT32)(this->line.textLength), // The length of the string.
      pTextFormat,                     // The text format to apply to the string (contains font information, etc).
      width,                           // The width of the layout box.
      height,                          // The height of the layout box.
      &this->line.pTextLayout          // The IDWriteTextLayout interface pointer.
  );

  if (hr == S_OK)
  {
    // Get maximum line length
    MJ_UNINITIALIZED DWRITE_TEXT_METRICS dtm;
    this->line.pTextLayout->GetMetrics(&dtm);
    const FLOAT lineWidth = dtm.widthIncludingTrailingWhitespace;
    if (lineWidth >= this->width)
    {
      this->width = lineWidth;
    }
  }

  return hr;
}
