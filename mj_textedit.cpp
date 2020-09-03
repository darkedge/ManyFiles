#include "mj_textedit.h"
#include <d2d1.h>
#include "mj_common.h"
#include "mj_win32.h"
#include "loremipsum.h"
#include "stretchy_buffer.h"
#include "render_target_resources.h"

static constexpr size_t BUFFER_SIZE   = 2 * 1024 * 1024; // 2 MiB
static constexpr FLOAT SCROLLBAR_SIZE = 20.0f;

HRESULT mj::TextEdit::Init(FLOAT left, FLOAT top, FLOAT right, FLOAT bottom)
{
  this->widgetRect.left   = left;
  this->widgetRect.top    = top;
  this->widgetRect.right  = right;
  this->widgetRect.bottom = bottom;

  this->scrollPos.x = 0.0f;
  this->scrollPos.y = 0.0f;

  this->pLines = nullptr;
  HRESULT hr   = S_OK;
  // Init memory for buffer
  this->pMemory = VirtualAlloc(0, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!this->pMemory)
  {
    hr = E_FAIL;
  }

  mj::GapBufferInit(&this->buf, this->pMemory, ((char*)this->pMemory) + BUFFER_SIZE);
  mj::GapBufferSetText(&this->buf, pLoremIpsum);

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
    mj::GapBufferInsertCharacterAtCursor(&this->buf, (wchar_t)wParam);
    break;
  case WM_KEYDOWN:
    switch (wParam)
    {
    case VK_HOME:
      mj::GapBufferJumpStartOfLine(&this->buf);
      break;
    case VK_END:
      mj::GapBufferJumpEndOfLine(&this->buf);
      break;
    case VK_LEFT:
      mj::GapBufferDecrementCursor(&this->buf);
      break;
    case VK_RIGHT:
      mj::GapBufferIncrementCursor(&this->buf);
      break;
    case VK_DELETE:
      mj::GapBufferDeleteAtCursor(&this->buf);
      break;
    case VK_BACK:
      mj::GapBufferBackspaceAtCursor(&this->buf);
      break;
    }
    break;
  }
}

static D2D1_POINT_2F operator*(const D2D1_MATRIX_3X2_F& matrix, const D2D1_POINT_2F& point)
{
  return D2D1::Matrix3x2F::ReinterpretBaseType(&matrix)->TransformPoint(point);
}

void mj::TextEdit::DrawHorizontalScrollBar(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F newTransform =
      transform * D2D1::Matrix3x2F::Translation(0.0f, this->widgetRect.bottom - this->widgetRect.top - SCROLLBAR_SIZE);
  pRenderTarget->SetTransform(MJ_REF newTransform);

  const auto widgetWidth = (this->widgetRect.right - this->widgetRect.left);

  // TODO: fix for high DPI
  MJ_UNINITIALIZED D2D1_RECT_F rect;
  rect.left   = 0.0f;
  rect.top    = 0.0f;
  rect.right  = widgetWidth;
  rect.bottom = SCROLLBAR_SIZE;
  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBackgroundBrush);

  rect.left  = this->scrollPos.x / this->width * widgetWidth;
  rect.right = (this->scrollPos.x + widgetWidth) / this->width * widgetWidth;

  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBrush);

  // Save for reverse lookup
  auto topLeft                   = newTransform * D2D1_POINT_2F{ rect.left, rect.top };
  auto bottomRight               = newTransform * D2D1_POINT_2F{ rect.right, rect.bottom };
  this->reverse.horScrollbarRect = RECT{ (LONG)topLeft.x, (LONG)topLeft.y, (LONG)bottomRight.x, (LONG)bottomRight.y };

  pRenderTarget->SetTransform(MJ_REF transform);
}

void mj::TextEdit::Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  // Background
  pRenderTarget->FillRectangle(MJ_REF this->widgetRect, pResources->pTextEditBackgroundBrush);
  pRenderTarget->PushAxisAlignedClip(MJ_REF this->widgetRect, D2D1_ANTIALIAS_MODE_ALIASED);

  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  pRenderTarget->SetTransform(transform * D2D1::Matrix3x2F::Translation(this->widgetRect.left, this->widgetRect.top));

  // Use the DrawTextLayout method of the D2D render target interface to draw.
  for (size_t i = 0; i < sb_count(this->pLines); i++)
  {
    MJ_UNINITIALIZED D2D1_POINT_2F inverse;
    inverse.x = -this->scrollPos.x;
    inverse.y = -this->scrollPos.y;
    pRenderTarget->DrawTextLayout(inverse, this->pLines[i].pTextLayout, pResources->pTextBrush,
                                  D2D1_DRAW_TEXT_OPTIONS_NONE);
  }
  this->DrawHorizontalScrollBar(pRenderTarget, pResources);
  // DrawCursor();

  pRenderTarget->PopAxisAlignedClip();
  pRenderTarget->SetTransform(MJ_REF transform);
}

static bool RectContainsPoint(D2D1_RECT_F* pRect, D2D1_POINT_2F* pPoint)
{
  return ((pPoint->x >= pRect->left) && (pPoint->x <= pRect->right) && (pPoint->y >= pRect->top) &&
          (pPoint->y < (pRect->bottom)));
}

void mj::TextEdit::MouseDown(SHORT x, SHORT y)
{
#if 0
  DWRITE_HIT_TEST_METRICS hitTestMetrics;
  BOOL isTrailingHit;
  BOOL isInside;

  this->lines[0]->HitTestPoint(((FLOAT)x) * s_BaseDpiScaleInv, ((FLOAT)y) / s_BaseDpiScaleInv, &isTrailingHit,
                                    &isInside, &hitTestMetrics);

  if (isInside == TRUE)
  {
    BOOL underline;
    this->lines[0]->GetUnderline(hitTestMetrics.textPosition, &underline);
    DWRITE_TEXT_RANGE textRange = { hitTestMetrics.textPosition, 1 };
    this->lines[0]->SetUnderline(!underline, textRange);
  }
#endif

  // Scroll bar
  POINT p{ (LONG)x, (LONG)y };
  if (PtInRect(&this->reverse.horScrollbarRect, p))
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
}

void mj::TextEdit::MouseUp(SHORT x, SHORT y)
{
  (void)x;
  (void)y;
  this->drag.draggable = mj::EDraggable::NONE;
}

mj::ECursor mj::TextEdit::MouseMove(SHORT x, SHORT y)
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

  wchar_t buf[1024]; // TODO: Small buffer!
  int numCharacters =
      mj::win32::Widen(buf, this->buf.pBufBegin, (int)(this->buf.pGapBegin - this->buf.pBufBegin), _countof(buf));
  numCharacters += mj::win32::Widen(&buf[numCharacters], this->buf.pGapEnd,
                                    (int)(this->buf.pBufEnd - this->buf.pGapEnd), _countof(buf) - numCharacters);

  for (int i = 0; i < sb_count(this->pLines); i++)
  {
    if (this->pLines[i].pTextLayout)
      this->pLines[i].pTextLayout->Release();
    delete[] this->pLines[i].pText;
  }
  sb_free(this->pLines);
  this->pLines = 0;
  TextEditLine line;
  line.pText = new wchar_t[numCharacters];
  memcpy(line.pText, buf, numCharacters * sizeof(wchar_t));
  line.textLength = numCharacters;
  sb_push(this->pLines, line);

  HRESULT hr = pFactory->CreateTextLayout(
      this->pLines[0].pText,                // The string to be laid out and formatted.
      (UINT32)(this->pLines[0].textLength), // The length of the string.
      pTextFormat,                          // The text format to apply to the string (contains font information, etc).
      width,                                // The width of the layout box.
      height,                               // The height of the layout box.
      &this->pLines[0].pTextLayout          // The IDWriteTextLayout interface pointer.
  );

  // Get maximum line length
  MJ_UNINITIALIZED DWRITE_TEXT_METRICS dtm;
  this->pLines[0].pTextLayout->GetMetrics(&dtm);
  const FLOAT lineWidth = dtm.widthIncludingTrailingWhitespace;
  if (lineWidth >= this->width)
  {
    this->width = lineWidth;
  }

  size_t position             = mj::GapBufferGetVirtualCursorPosition(&this->buf);
  DWRITE_TEXT_RANGE textRange = { (UINT32)position, 1 };
  this->pLines[0].pTextLayout->SetUnderline(true, textRange);

  return hr;
}
