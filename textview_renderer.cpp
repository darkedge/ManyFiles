#include "textview.h"
#include <d2d1.h>
#include "render_target_resources.h"
#include "mj_common.h"

HRESULT mj::TextView::TextViewRenderer::DrawGlyphRun(
    void* clientDrawingContext,                              //
    FLOAT baselineOriginX,                                   //
    FLOAT baselineOriginY,                                   //
    DWRITE_MEASURING_MODE measuringMode,                     //
    DWRITE_GLYPH_RUN const* glyphRun,                        //
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription, //
    IUnknown* clientDrawingEffect) noexcept                  // Brush set with SetDrawingEffect
{
  HRESULT hr = S_OK;
  MJ_DISCARD(glyphRunDescription);
  // Break out DrawingContext fields
  DrawingContext* drawingContext = static_cast<DrawingContext*>(clientDrawingContext);

  mj::ComPtr<ID2D1RenderTarget> pRenderTarget;
  mj::ComPtr<ID2D1Brush> foregroundBrush;
  mj::ComPtr<ID2D1Brush> backgroundBrush;

  if (SUCCEEDED(hr))
  {
    hr = drawingContext->pRenderTarget.As(&pRenderTarget);
  }
  if (SUCCEEDED(hr))
  {
    hr = drawingContext->pResources->pTextBrush.As(&foregroundBrush);
  }
  if (SUCCEEDED(hr))
  {
    hr = drawingContext->pResources->pScrollBarBrush.As(&backgroundBrush);
  }
  if (SUCCEEDED(hr))
  {
    // Get width of text
    FLOAT totalWidth = 0;

    for (UINT32 index = 0; index < glyphRun->glyphCount; index++)
    {
      totalWidth += glyphRun->glyphAdvances[index];
    }

    // Get height of text
    MJ_UNINITIALIZED DWRITE_FONT_METRICS fontMetrics;
    glyphRun->fontFace->GetMetrics(&fontMetrics);
    FLOAT adjust  = glyphRun->fontEmSize / fontMetrics.designUnitsPerEm;
    FLOAT ascent  = adjust * fontMetrics.ascent;
    FLOAT descent = adjust * fontMetrics.descent;
    D2D1_RECT_F rect =
        D2D1::RectF(baselineOriginX, baselineOriginY - ascent, baselineOriginX + totalWidth, baselineOriginY + descent);

    // Fill Rectangle
    pRenderTarget->FillRectangle(rect, backgroundBrush.Get());

    pRenderTarget->DrawGlyphRun(D2D1::Point2F(baselineOriginX, baselineOriginY), glyphRun, foregroundBrush.Get(),
                                measuringMode);
  }

  return hr;
}

HRESULT mj::TextView::TextViewRenderer::DrawUnderline(void* clientDrawingContext,        //
                                                      FLOAT baselineOriginX,             //
                                                      FLOAT baselineOriginY,             //
                                                      DWRITE_UNDERLINE const* underline, //
                                                      IUnknown* clientDrawingEffect) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(baselineOriginX);
  MJ_DISCARD(baselineOriginY);
  MJ_DISCARD(underline);
  MJ_DISCARD(clientDrawingEffect);

  return E_NOTIMPL;
}

HRESULT mj::TextView::TextViewRenderer::DrawStrikethrough(void* clientDrawingContext,                //
                                                          FLOAT baselineOriginX,                     //
                                                          FLOAT baselineOriginY,                     //
                                                          DWRITE_STRIKETHROUGH const* strikethrough, //
                                                          IUnknown* clientDrawingEffect) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(baselineOriginX);
  MJ_DISCARD(baselineOriginY);
  MJ_DISCARD(strikethrough);
  MJ_DISCARD(clientDrawingEffect);

  return E_NOTIMPL;
}

HRESULT mj::TextView::TextViewRenderer::DrawInlineObject(void* clientDrawingContext,        //
                                                         FLOAT originX,                     //
                                                         FLOAT originY,                     //
                                                         IDWriteInlineObject* inlineObject, //
                                                         BOOL isSideways,                   //
                                                         BOOL isRightToLeft,                //
                                                         IUnknown* clientDrawingEffect) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(originX);
  MJ_DISCARD(originY);
  MJ_DISCARD(inlineObject);
  MJ_DISCARD(isSideways);
  MJ_DISCARD(isRightToLeft);
  MJ_DISCARD(clientDrawingEffect);

  return E_NOTIMPL;
}

// IDWritePixelSnapping
HRESULT mj::TextView::TextViewRenderer::IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(isDisabled);

  return S_OK;
}

HRESULT mj::TextView::TextViewRenderer::GetCurrentTransform(void* clientDrawingContext,
                                                            DWRITE_MATRIX* transform) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(transform);

  return S_OK;
}

HRESULT mj::TextView::TextViewRenderer::GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) noexcept
{
  MJ_DISCARD(clientDrawingContext);
  MJ_DISCARD(pixelsPerDip);

  return S_OK;
}

// IUnknown
HRESULT mj::TextView::TextViewRenderer::QueryInterface(REFIID riid, void** ppvObject) noexcept
{
  MJ_DISCARD(riid);
  MJ_DISCARD(ppvObject);

  return S_OK;
}

ULONG mj::TextView::TextViewRenderer::AddRef() noexcept
{
  return 0;
}

ULONG mj::TextView::TextViewRenderer::Release() noexcept
{
  return 0;
}
