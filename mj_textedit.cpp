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

  this->text.Init();

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
  this->pParent = pParent;
  this->back    = {};
  this->front   = {};
}

void mj::HorizontalScrollBar::Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  D2D1_RECT_F widgetRect = this->pParent->GetWidgetRect();
  FLOAT scrollPos        = this->pParent->GetScrollPosition().x;

  const auto widgetWidth = (widgetRect.right - widgetRect.left);

  FLOAT y = widgetRect.bottom - widgetRect.top - SCROLLBAR_SIZE;

  // TODO: fix for high DPI
  back.left   = 0.0f;
  back.top    = y;
  back.right  = widgetWidth;
  back.bottom = y + SCROLLBAR_SIZE;
  pRenderTarget->FillRectangle(MJ_REF back, pResources->pScrollBarBackgroundBrush);

  front = back;

  front.left  = scrollPos / this->pParent->GetWidth() * widgetWidth;
  front.right = (scrollPos + widgetWidth) / this->pParent->GetWidth() * widgetWidth;

  pRenderTarget->FillRectangle(MJ_REF front, pResources->pScrollBarBrush);
}

static RECT ToRect(const D2D_RECT_F& rectf)
{
  return RECT{
    (LONG)rectf.left,
    (LONG)rectf.top,
    (LONG)rectf.right,
    (LONG)rectf.bottom,
  };
}

bool mj::HorizontalScrollBar::MouseDown(SHORT x, SHORT y)
{
  POINT pt{ (LONG)x, (LONG)y };
  RECT rect = ToRect(this->front);
  return PtInRect(&rect, pt);
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

  pRenderTarget->SetTransform(xWidget * D2D1::Matrix3x2F::Translation(-this->scrollAmount.x, -this->scrollAmount.y));
  this->text.Draw(pRenderTarget, pResources, this->buf.GetVirtualCaretPosition());

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
  // Translate
  x -= (SHORT)this->widgetRect.left;
  y -= (SHORT)this->widgetRect.top;

  // Scroll bar
  if (this->horizontalScrollBar.MouseDown(x, y))
  {
    // Use left edge of scroll bar
    this->drag.start       = this->scrollAmount.x;
    this->drag.mouseStartX = x;
    // this->drag.mouseStartY = y;
    this->drag.draggable = EDraggable::HOR_SCROLLBAR;
  }
  else
  {
    this->drag.draggable = EDraggable::NONE;
  }

  // Caret
  MJ_UNINITIALIZED UINT32 textPosition;
  if (this->text.MouseDown(x, y, MJ_REF textPosition))
  {
    this->buf.SetCaretPosition(textPosition);
  }
}

void mj::TextEdit::MouseUp()
{
  this->drag.draggable = mj::EDraggable::NONE;
}

mj::ECursor::Enum mj::TextEdit::MouseMove(SHORT x, SHORT y)
{
  // Translate
  SHORT xRel = x - (SHORT)this->widgetRect.left;
  SHORT yRel = y - (SHORT)this->widgetRect.top;

  // Check dragging
  switch (this->drag.draggable)
  {
  case EDraggable::HOR_SCROLLBAR:
  {
    const auto widgetWidth = (this->widgetRect.right - this->widgetRect.left);
    SHORT dx               = xRel - this->drag.mouseStartX;
    this->scrollAmount.x   = this->drag.start + (dx / widgetWidth * this->width);
    if (this->scrollAmount.x < 0.0f)
    {
      this->scrollAmount.x = 0.0f;
    }
    else if ((this->scrollAmount.x + widgetWidth) > this->width)
    {
      this->scrollAmount.x = (this->width - widgetWidth);
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

  HRESULT hr = this->text.CreateDeviceResources(pFactory, pTextFormat, this->buf, width, height);

  if (hr == S_OK)
  {
    FLOAT lineWidth = this->text.GetWidth();
    if (lineWidth >= this->width)
    {
      this->width = lineWidth;
    }
  }

  return hr;
}

void mj::TextView::Init()
{
  this->pText       = nullptr;
  this->textLength  = 0;
  this->pTextLayout = nullptr;
  this->pTextBrush  = nullptr;
}

void mj::TextView::Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources, UINT32 textPosition)
{
  if (!this->pTextLayout)
    return;

  pRenderTarget->DrawTextLayout({}, this->pTextLayout, pResources->pTextBrush, D2D1_DRAW_TEXT_OPTIONS_NONE);

  // Caret
  DWRITE_HIT_TEST_METRICS hitTestMetrics;
  float caretX, caretY;
  bool isTrailingHit = false; // Use the leading character edge for simplicity here.

  // Map text position index to caret coordinate and hit-test rectangle.
  MJ_DISCARD(this->pTextLayout->HitTestTextPosition(textPosition, isTrailingHit, &caretX, &caretY, &hitTestMetrics));
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

HRESULT mj::TextView::CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                            const mj::GapBuffer& buffer, FLOAT width, FLOAT height)
{
  int numWideCharsLeft  = buffer.GetLeftLength();
  int numWideCharsRight = buffer.GetRightLength();
  int numWideCharsTotal = numWideCharsLeft + numWideCharsRight;

  // Delete all rendered lines
  if (this->pTextLayout)
  {
    this->pTextLayout->Release();
  }
  delete[] this->pText;

  wchar_t* pText = new wchar_t[numWideCharsTotal];
  if (pText)
  {
    CopyMemory(pText, buffer.GetLeftPtr(), numWideCharsLeft * sizeof(wchar_t));
    CopyMemory(pText + numWideCharsLeft, buffer.GetRightPtr(), numWideCharsRight * sizeof(wchar_t));
  }

  this->pText      = pText;
  this->textLength = numWideCharsTotal;

  return pFactory->CreateTextLayout(
      this->pText,                // The string to be laid out and formatted.
      (UINT32)(this->textLength), // The length of the string.
      pTextFormat,                // The text format to apply to the string (contains font information, etc).
      width,                      // The width of the layout box.
      height,                     // The height of the layout box.
      &this->pTextLayout          // The IDWriteTextLayout interface pointer.
  );
}

FLOAT mj::TextView::GetWidth() const
{
  // Get maximum line length
  MJ_UNINITIALIZED DWRITE_TEXT_METRICS dtm;
  this->pTextLayout->GetMetrics(&dtm);
  return dtm.widthIncludingTrailingWhitespace;
}

bool mj::TextView::MouseDown(SHORT x, SHORT y, UINT32& textPosition)
{
  if (!this->pTextLayout)
    return false;

  MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
  MJ_UNINITIALIZED BOOL isTrailingHit;
  MJ_UNINITIALIZED BOOL isInside;

  MJ_DISCARD(this->pTextLayout->HitTestPoint(((FLOAT)x), ((FLOAT)y), &isTrailingHit, &isInside, &hitTestMetrics));

  if (isInside)
  {
    textPosition = hitTestMetrics.textPosition;
    return true;
  }

  return false;
}