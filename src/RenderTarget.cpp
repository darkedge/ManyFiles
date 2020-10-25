#include "..\3rdparty\tracy\Tracy.hpp"
#include "Common.h"
#include "DrawingEffect.h"
#include "RenderTarget.h"

HRESULT RenderTarget::Create(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd,
                             OUT RenderTarget** renderTarget)
{
  ZoneScoped;
  *renderTarget = nullptr;
  HRESULT hr    = mj::kOK;

  RenderTarget* newRenderTarget =
      new RenderTarget(d2dFactory, dwriteFactory, hwnd); // TODO MJ: Untracked memory allocation
  if (!newRenderTarget)
  {
    return mj::EOutOfMemory;
  }

  hr = newRenderTarget->CreateTarget();
  if (mj::Failed(hr))
    delete newRenderTarget;

  *renderTarget = newRenderTarget;

  return hr;
}

RenderTarget::RenderTarget(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd)
    : hwnd_(hwnd), pD2dFactory(d2dFactory), pDWriteFactory(dwriteFactory)
{
}

HRESULT RenderTarget::CreateTarget()
{
  ZoneScoped;
  // Creates a D2D render target set on the HWND.

  // Get the window's pixel size.
  RECT rect = {};
  static_cast<void>(GetClientRect(hwnd_, &rect));
  const D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Create a D2D render target.
  ID2D1HwndRenderTarget* pRenderTarget = nullptr;

  HRESULT hr = this->pD2dFactory->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                                 D2D1::HwndRenderTargetProperties(hwnd_, d2dSize), &pRenderTarget);

  if (mj::Succeeded(hr))
  {
    this->pRenderTarget = pRenderTarget;

    // Any scaling will be combined into matrix transforms rather than an
    // additional DPI scaling. This simplifies the logic for rendering
    // and hit-testing. If an application does not use matrices, then
    // using the scaling factor directly is simpler.
    pRenderTarget->SetDpi(96.0, 96.0);

    // Create a reusable scratch brush, rather than allocating one for
    // each new color.
    hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), this->pBrush.ReleaseAndGetAddressOf());
  }

  if (mj::Succeeded(hr))
  {
    // Update the initial monitor rendering parameters.
    this->UpdateMonitor();
  }

  return hr;
}

void RenderTarget::Resize(UINT width, UINT height)
{
  MJ_UNINITIALIZED D2D1_SIZE_U size;
  size.width  = width;
  size.height = height;
  this->pRenderTarget->Resize(size);
}

void RenderTarget::UpdateMonitor()
{
  ZoneScoped;
  // Updates rendering parameters according to current monitor.

  HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
  if (monitor != hmonitor_)
  {
    // Create based on monitor settings, rather than the defaults of
    // gamma=1.8, contrast=.5, and clearTypeLevel=.5

    mj::ComPtr<IDWriteRenderingParams> renderingParams;

    this->pDWriteFactory->CreateMonitorRenderingParams(monitor, renderingParams.GetAddressOf());
    this->pRenderTarget->SetTextRenderingParams(renderingParams.Get());

    hmonitor_ = monitor;
    InvalidateRect(hwnd_, nullptr, FALSE);
  }
}

void RenderTarget::BeginDraw()
{
  ZoneScoped;
  this->pRenderTarget->BeginDraw();
  this->pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}

void RenderTarget::EndDraw()
{
  ZoneScoped;
  const HRESULT hr = this->pRenderTarget->EndDraw();

  // If the device is lost for any reason, we need to recreate it.
  if (hr == mj::D2dErrRecreateTarget)
  {
    // Flush resources and recreate them.
    // This is very rare for a device to be lost,
    // but it can occur when connecting via Remote Desktop.
    // imageCache_.Clear();
    hmonitor_ = nullptr;

    CreateTarget();
  }
}

void RenderTarget::Clear(UINT32 color)
{
  ZoneScoped;
  this->pRenderTarget->Clear(D2D1::ColorF(color));
}

void RenderTarget::FillRectangle(const RectF& destRect, const DrawingEffect& drawingEffect)
{
  ZoneScoped;
  ID2D1Brush* brush = GetCachedBrush(&drawingEffect);
  if (!brush)
    return;

  // We will always get a strikethrough as a LTR rectangle
  // with the baseline origin snapped.
  this->pRenderTarget->FillRectangle(destRect, brush);
}

void RenderTarget::DrawTextLayout(IDWriteTextLayout* textLayout, const RectF& rect)
{
  ZoneScoped;
  if (!textLayout)
    return;

  Context context(this, nullptr);
  textLayout->Draw(&context, this, rect.left, rect.top);
}

ID2D1Brush* RenderTarget::GetCachedBrush(const DrawingEffect* effect)
{
  ZoneScoped;
  if (!effect || !this->pBrush)
    return nullptr;

  // Update the D2D brush to the new effect color.
  const UINT32 bgra = effect->GetColor();
  const float alpha = (bgra >> 24) / 255.0f;
  this->pBrush->SetColor(D2D1::ColorF(bgra, alpha));

  return this->pBrush.Get();
}

void RenderTarget::SetTransform(DWRITE_MATRIX const& transform)
{
  ZoneScoped;
  this->pRenderTarget->SetTransform(reinterpret_cast<const D2D1_MATRIX_3X2_F*>(&transform));
}

void RenderTarget::GetTransform(DWRITE_MATRIX& transform)
{
  ZoneScoped;
  this->pRenderTarget->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(&transform));
}

void RenderTarget::SetAntialiasing(bool isEnabled)
{
  ZoneScoped;
  this->pRenderTarget->SetAntialiasMode(isEnabled ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

HRESULT STDMETHODCALLTYPE RenderTarget::DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX,
                                                     FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode,
                                                     const DWRITE_GLYPH_RUN* glyphRun,
                                                     const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
                                                     IUnknown* clientDrawingEffect)
{
  ZoneScoped;
  // If no drawing effect is applied to run, but a clientDrawingContext
  // is passed, use the one from that instead. This is useful for trimming
  // signs, where they don't have a color of their own.
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  // Since we use our own custom renderer and explicitly set the effect
  // on the layout, we know exactly what the parameter is and can
  // safely cast it directly.
  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (!brush)
    return mj::EFail;

  this->pRenderTarget->DrawGlyphRun(D2D1::Point2(baselineOriginX, baselineOriginY), glyphRun, brush, measuringMode);

  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::DrawUnderline(void* clientDrawingContext, FLOAT baselineOriginX,
                                                      FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline,
                                                      IUnknown* clientDrawingEffect)
{
  ZoneScoped;
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (!brush)
    return mj::EFail;

  // We will always get a strikethrough as a LTR rectangle
  // with the baseline origin snapped.
  D2D1_RECT_F rectangle = { baselineOriginX, baselineOriginY + underline->offset, baselineOriginX + underline->width,
                            baselineOriginY + underline->offset + underline->thickness };

  // Draw this as a rectangle, rather than a line.
  this->pRenderTarget->FillRectangle(&rectangle, brush);

  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::DrawStrikethrough(void* clientDrawingContext, FLOAT baselineOriginX,
                                                          FLOAT baselineOriginY,
                                                          const DWRITE_STRIKETHROUGH* strikethrough,
                                                          IUnknown* clientDrawingEffect)
{
  ZoneScoped;
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (!brush)
    return mj::EFail;

  // We will always get an underline as a LTR rectangle
  // with the baseline origin snapped.
  D2D1_RECT_F rectangle = { baselineOriginX, baselineOriginY + strikethrough->offset,
                            baselineOriginX + strikethrough->width,
                            baselineOriginY + strikethrough->offset + strikethrough->thickness };

  // Draw this as a rectangle, rather than a line.
  this->pRenderTarget->FillRectangle(&rectangle, brush);

  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::DrawInlineObject(void* clientDrawingContext, FLOAT originX, FLOAT originY,
                                                         IDWriteInlineObject* inlineObject, BOOL isSideways,
                                                         BOOL isRightToLeft, IUnknown* clientDrawingEffect)
{
  ZoneScoped;
  // Inline objects inherit the drawing effect of the text
  // they are in, so we should pass it down (if none is set
  // on this range, use the drawing context's effect instead).
  Context subContext(*reinterpret_cast<RenderTarget::Context*>(clientDrawingContext));

  if (clientDrawingEffect)
    subContext.pDrawingEffect = clientDrawingEffect;

  inlineObject->Draw(&subContext, this, originX, originY, false, false, subContext.pDrawingEffect);

  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::IsPixelSnappingDisabled(void* clientDrawingContext, OUT BOOL* isDisabled)
{
  ZoneScoped;
  // Enable pixel snapping of the text baselines,
  // since we're not animating and don't want blurry text.
  *isDisabled = FALSE;
  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::GetCurrentTransform(void* clientDrawingContext, OUT DWRITE_MATRIX* transform)
{
  ZoneScoped;
  // Simply forward what the real renderer holds onto.
  this->pRenderTarget->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
  return mj::kOK;
}

HRESULT STDMETHODCALLTYPE RenderTarget::GetPixelsPerDip(void* clientDrawingContext, OUT FLOAT* pixelsPerDip)
{
  ZoneScoped;
  // Any scaling will be combined into matrix transforms rather than an
  // additional DPI scaling. This simplifies the logic for rendering
  // and hit-testing. If an application does not use matrices, then
  // using the scaling factor directly is simpler.
  *pixelsPerDip = 1;
  return mj::kOK;
}
