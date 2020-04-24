#include "mj_textedit.h"
#include <d2d1.h>
#include "mj_common.h"
#include "mj_win32.h"
#include "loremipsum.h"
#include "stretchy_buffer.h"
#include "render_target_resources.h"

static constexpr size_t BUFFER_SIZE   = 2 * 1024 * 1024; // 2 MiB
static constexpr FLOAT SCROLLBAR_SIZE = 20.0f;

HRESULT mj::TextEditInit(TextEdit* pTextEdit, FLOAT left, FLOAT top, FLOAT right, FLOAT bottom)
{
  pTextEdit->widgetRect.left   = left;
  pTextEdit->widgetRect.top    = top;
  pTextEdit->widgetRect.right  = right;
  pTextEdit->widgetRect.bottom = bottom;

  pTextEdit->scrollPos.x = 0.0f;
  pTextEdit->scrollPos.y = 0.0f;

  pTextEdit->pLines = nullptr;
  HRESULT hr        = S_OK;
  // Init memory for buffer
  pTextEdit->pMemory = VirtualAlloc(0, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (!pTextEdit->pMemory)
  {
    hr = E_FAIL;
  }

  mj::GapBufferInit(&pTextEdit->buf, pTextEdit->pMemory, ((char*)pTextEdit->pMemory) + BUFFER_SIZE);
  mj::GapBufferSetText(&pTextEdit->buf, pLoremIpsum);

  return hr;
}

void mj::TextEditDestroy(TextEdit* pTextEdit)
{
  if (pTextEdit->pMemory)
  {
    MJ_DISCARD(VirtualFree(pTextEdit->pMemory, 0, MEM_RELEASE));
    pTextEdit->pMemory = nullptr;
  }
}

void mj::TextEditWndProc(TextEdit* pTextEdit, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  (void)lParam;
  (void)hwnd;
  switch (message)
  {
  case WM_CHAR:
    mj::GapBufferInsertCharacterAtCursor(&pTextEdit->buf, (wchar_t)wParam);
    break;
  case WM_KEYDOWN:
    switch (wParam)
    {
    case VK_HOME:
      mj::GapBufferJumpStartOfLine(&pTextEdit->buf);
      break;
    case VK_END:
      mj::GapBufferJumpEndOfLine(&pTextEdit->buf);
      break;
    case VK_LEFT:
      mj::GapBufferDecrementCursor(&pTextEdit->buf);
      break;
    case VK_RIGHT:
      mj::GapBufferIncrementCursor(&pTextEdit->buf);
      break;
    case VK_DELETE:
      mj::GapBufferDeleteAtCursor(&pTextEdit->buf);
      break;
    case VK_BACK:
      mj::GapBufferBackspaceAtCursor(&pTextEdit->buf);
      break;
    }
    break;
  }
}

static void DrawHorizontalScrollBar(mj::TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget,
                                    RenderTargetResources* pResources)
{
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  pRenderTarget->SetTransform(
      transform *
      D2D1::Matrix3x2F::Translation(0.0f, pTextEdit->widgetRect.bottom - pTextEdit->widgetRect.top - SCROLLBAR_SIZE));

  // TODO: fix for high DPI
  MJ_UNINITIALIZED D2D1_RECT_F rect;
  rect.left   = 0.0f;
  rect.right  = pTextEdit->widgetRect.right - pTextEdit->widgetRect.left;
  rect.top    = 0.0f;
  rect.bottom = SCROLLBAR_SIZE;
  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBackgroundBrush);

  rect.left         = pTextEdit->scrollPos.x * pTextEdit->scrollPos.x / pTextEdit->width;
  const float right = (pTextEdit->scrollPos.x + (pTextEdit->widgetRect.right - pTextEdit->widgetRect.left));
  rect.right        = right * right / pTextEdit->width;
  pRenderTarget->FillRectangle(MJ_REF rect, pResources->pScrollBarBrush);

  pRenderTarget->SetTransform(MJ_REF transform);
}

void mj::TextEditDraw(TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources)
{
  // Background
  pRenderTarget->FillRectangle(MJ_REF pTextEdit->widgetRect, pResources->pTextEditBackgroundBrush);
  pRenderTarget->PushAxisAlignedClip(MJ_REF pTextEdit->widgetRect, D2D1_ANTIALIAS_MODE_ALIASED);

  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  pRenderTarget->SetTransform(transform *
                              D2D1::Matrix3x2F::Translation(pTextEdit->widgetRect.left, pTextEdit->widgetRect.top));

  // Use the DrawTextLayout method of the D2D render target interface to draw.
  for (size_t i = 0; i < sb_count(pTextEdit->pLines); i++)
  {
    pRenderTarget->DrawTextLayout(D2D1_POINT_2F{ 0.0f, 0.0f }, pTextEdit->pLines[i].pTextLayout, pResources->pTextBrush,
                                  D2D1_DRAW_TEXT_OPTIONS_CLIP);
  }
  DrawHorizontalScrollBar(pTextEdit, pRenderTarget, pResources);
  // DrawCursor();

  pRenderTarget->PopAxisAlignedClip();
  pRenderTarget->SetTransform(MJ_REF transform);
}

void mj::TextEditOnClick(TextEdit* pTextEdit, UINT x, UINT y)
{
  (void)pTextEdit;
  (void)x;
  (void)y;
#if 0
  DWRITE_HIT_TEST_METRICS hitTestMetrics;
  BOOL isTrailingHit;
  BOOL isInside;

  pTextEdit->lines[0]->HitTestPoint(((FLOAT)x) * s_BaseDpiScaleInv, ((FLOAT)y) / s_BaseDpiScaleInv, &isTrailingHit,
                                    &isInside, &hitTestMetrics);

  if (isInside == TRUE)
  {
    BOOL underline;
    pTextEdit->lines[0]->GetUnderline(hitTestMetrics.textPosition, &underline);
    DWRITE_TEXT_RANGE textRange = { hitTestMetrics.textPosition, 1 };
    pTextEdit->lines[0]->SetUnderline(!underline, textRange);
  }
#endif
}

static bool RectContainsPoint(D2D1_RECT_F* pRect, D2D1_POINT_2F* pPoint)
{
  return ((pPoint->x >= pRect->left) && (pPoint->x <= pRect->right) && (pPoint->y >= pRect->top) &&
          (pPoint->y < (pRect->bottom - SCROLLBAR_SIZE)));
}

mj::ECursor mj::TextEditMouseMove(TextEdit* pTextEdit, int x, int y)
{
  MJ_UNINITIALIZED D2D1_POINT_2F p;
  p.x = (FLOAT)x;
  p.y = (FLOAT)y;
  if (RectContainsPoint(&pTextEdit->widgetRect, &p))
  {
    return mj::ECursor::IBEAM;
  }
  return mj::ECursor::ARROW;
}

HRESULT mj::TextEditCreateDeviceResources(TextEdit* pTextEdit, IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                          float width, float height)
{
  // Set longest line width equal to widget width
  pTextEdit->width = (pTextEdit->widgetRect.right - pTextEdit->widgetRect.left);

  wchar_t buf[1024]; // TODO: Small buffer!
  int numCharacters = mj::win32::Widen(buf, pTextEdit->buf.pBufBegin,
                                       (int)(pTextEdit->buf.pGapBegin - pTextEdit->buf.pBufBegin), _countof(buf));
  numCharacters +=
      mj::win32::Widen(&buf[numCharacters], pTextEdit->buf.pGapEnd,
                       (int)(pTextEdit->buf.pBufEnd - pTextEdit->buf.pGapEnd), _countof(buf) - numCharacters);

  for (int i = 0; i < sb_count(pTextEdit->pLines); i++)
  {
    if (pTextEdit->pLines[i].pTextLayout)
      pTextEdit->pLines[i].pTextLayout->Release();
    delete[] pTextEdit->pLines[i].pText;
  }
  sb_free(pTextEdit->pLines);
  pTextEdit->pLines = 0;
  TextEditLine line;
  line.pText = new wchar_t[numCharacters];
  memcpy(line.pText, buf, numCharacters * sizeof(wchar_t));
  line.textLength = numCharacters;
  sb_push(pTextEdit->pLines, line);

  HRESULT hr = pFactory->CreateTextLayout(
      pTextEdit->pLines[0].pText,                // The string to be laid out and formatted.
      (UINT32)(pTextEdit->pLines[0].textLength), // The length of the string.
      pTextFormat,                      // The text format to apply to the string (contains font information, etc).
      width,                            // The width of the layout box.
      height,                           // The height of the layout box.
      &pTextEdit->pLines[0].pTextLayout // The IDWriteTextLayout interface pointer.
  );

  // Get maximum line length
  MJ_UNINITIALIZED DWRITE_TEXT_METRICS dtm;
  pTextEdit->pLines[0].pTextLayout->GetMetrics(&dtm);
  const FLOAT lineWidth = dtm.widthIncludingTrailingWhitespace;
  if (lineWidth >= pTextEdit->width)
  {
    pTextEdit->width = lineWidth;
  }

  size_t position             = mj::GapBufferGetVirtualCursorPosition(&pTextEdit->buf);
  DWRITE_TEXT_RANGE textRange = { (UINT32)position, 1 };
  pTextEdit->pLines[0].pTextLayout->SetUnderline(true, textRange);

  return hr;
}