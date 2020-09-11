#include "textview.h"
#include "mj_gapbuffer.h"
#include <d2d1.h>
#include "mj_common.h"
#include "render_target_resources.h"

void mj::TextView::Init() noexcept
{
  this->pText         = nullptr;
  this->textLength    = 0;
  this->pTextLayout   = nullptr;
  this->hoverPosition = 0;
  this->pRenderer     = new (this->renderer_storage) TextViewRenderer();
}

void mj::TextView::Draw(mj::ComPtr<ID2D1HwndRenderTarget> pRenderTarget, RenderTargetResources* pResources,
                        UINT32 textPosition) noexcept
{
  if (!this->pTextLayout)
    return;

  MJ_UNINITIALIZED TextViewRenderer::DrawingContext drawingContext;
  drawingContext.pRenderTarget = pRenderTarget;
  drawingContext.pResources    = pResources;
  MJ_DISCARD(this->pTextLayout->Draw(&drawingContext, this->pRenderer, 0.0f, 0.0f));
  MJ_DISCARD(&TextViewRenderer::DrawGlyphRun); // Visual Studio shortcut

  // Caret
  MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
  MJ_UNINITIALIZED float caretX, caretY;

  // Map text position index to caret coordinate and hit-test rectangle.
  MJ_DISCARD(this->pTextLayout->HitTestTextPosition(textPosition, false, &caretX, &caretY, &hitTestMetrics));
  // Respect user settings.
  DWORD caretWidth = 1;
  MJ_DISCARD(SystemParametersInfoW(SPI_GETCARETWIDTH, 0, &caretWidth, 0));
  DWORD halfCaretWidth = caretWidth / 2u;

  // Draw a thin rectangle.
  MJ_UNINITIALIZED D2D1_RECT_F rect;
  rect.left   = caretX - halfCaretWidth;
  rect.top    = hitTestMetrics.top;
  rect.right  = caretX + (caretWidth - halfCaretWidth);
  rect.bottom = hitTestMetrics.top + hitTestMetrics.height;

  pRenderTarget->FillRectangle(&rect, pResources->pCaretBrush.Get());
}

void mj::TextView::MouseMove(SHORT x, SHORT y) noexcept
{
  MJ_DISCARD(this->GetTextPosition(x, y, MJ_REF this->hoverPosition));
}

HRESULT mj::TextView::CreateDeviceResources(mj::ComPtr<IDWriteFactory> pFactory,
                                            mj::ComPtr<IDWriteTextFormat> pTextFormat, const mj::GapBuffer& buffer,
                                            FLOAT width, FLOAT height) noexcept
{
  int numWideCharsLeft  = buffer.GetLeftLength();
  int numWideCharsRight = buffer.GetRightLength();
  int numWideCharsTotal = numWideCharsLeft + numWideCharsRight;

  // Delete layout text
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
      pTextFormat.Get(),          // The text format to apply to the string (contains font information, etc).
      width,                      // The width of the layout box.
      height,                     // The height of the layout box.
      this->pTextLayout.ReleaseAndGetAddressOf() // The IDWriteTextLayout interface pointer.
  );
}

FLOAT mj::TextView::GetWidth() const noexcept
{
  // Get maximum line length
  MJ_UNINITIALIZED DWRITE_TEXT_METRICS dtm;
  this->pTextLayout->GetMetrics(&dtm);
  return dtm.widthIncludingTrailingWhitespace;
}

bool mj::TextView::GetTextPosition(SHORT x, SHORT y, UINT32& textPosition) noexcept
{
  if (!this->pTextLayout)
    return false;

  MJ_UNINITIALIZED DWRITE_HIT_TEST_METRICS hitTestMetrics;
  MJ_UNINITIALIZED BOOL isTrailingHit;
  MJ_UNINITIALIZED BOOL isInside;

  MJ_DISCARD(this->pTextLayout->HitTestPoint(((FLOAT)x), ((FLOAT)y), &isTrailingHit, &isInside, &hitTestMetrics));

  textPosition = isTrailingHit ? hitTestMetrics.textPosition + 1 : hitTestMetrics.textPosition;
  return true;
}
