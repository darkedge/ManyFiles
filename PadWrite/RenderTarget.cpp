// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// Contents:    Adapter render target draws using D2D or DirectWrite.
//              This demonstrates how to implement your own render target
//              for layout drawing callbacks.
//
//----------------------------------------------------------------------------
#include "Common.h"
#include "DrawingEffect.h"
#include "RenderTarget.h"

#if 0
inline bool operator== (const RenderTargetD2D::ImageCacheEntry& entry, const IWICBitmapSource* original)
{
    return entry.original == original;
}

inline bool operator== (const RenderTargetDW::ImageCacheEntry& entry, const IWICBitmapSource* original)
{
    return entry.original == original;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// Direct2D render target.

HRESULT RenderTargetD2D::Create(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd,
                                OUT RenderTarget** renderTarget)
{
  *renderTarget = nullptr;
  HRESULT hr    = S_OK;

  RenderTargetD2D* newRenderTarget = SafeAcquire(new RenderTargetD2D(d2dFactory, dwriteFactory, hwnd));
  if (newRenderTarget == nullptr)
  {
    return E_OUTOFMEMORY;
  }

  hr = newRenderTarget->CreateTarget();
  if (FAILED(hr))
    SafeRelease(&newRenderTarget);

  *renderTarget = SafeDetach(&newRenderTarget);

  return hr;
}

RenderTargetD2D::RenderTargetD2D(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd)
    : hwnd_(hwnd), hmonitor_(nullptr), d2dFactory_(SafeAcquire(d2dFactory)), dwriteFactory_(SafeAcquire(dwriteFactory)),
      target_(), brush_()
{
}

RenderTargetD2D::~RenderTargetD2D()
{
  SafeRelease(&brush_);
  SafeRelease(&target_);
  SafeRelease(&d2dFactory_);
  SafeRelease(&dwriteFactory_);
}

HRESULT RenderTargetD2D::CreateTarget()
{
  // Creates a D2D render target set on the HWND.

  HRESULT hr = S_OK;

  // Get the window's pixel size.
  RECT rect = {};
  GetClientRect(hwnd_, &rect);
  D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Create a D2D render target.
  ID2D1HwndRenderTarget* target = nullptr;

  hr = d2dFactory_->CreateHwndRenderTarget(D2D1::RenderTargetProperties(),
                                           D2D1::HwndRenderTargetProperties(hwnd_, d2dSize), &target);

  if (SUCCEEDED(hr))
  {
    SafeSet(&target_, target);

    // Any scaling will be combined into matrix transforms rather than an
    // additional DPI scaling. This simplifies the logic for rendering
    // and hit-testing. If an application does not use matrices, then
    // using the scaling factor directly is simpler.
    target->SetDpi(96.0, 96.0);

    // Create a reusable scratch brush, rather than allocating one for
    // each new color.
    SafeRelease(&brush_);
    hr = target->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &brush_);
  }

  if (SUCCEEDED(hr))
  {
    // Update the initial monitor rendering parameters.
    UpdateMonitor();
  }

  SafeRelease(&target);

  return hr;
}

void RenderTargetD2D::Resize(UINT width, UINT height)
{
  D2D1_SIZE_U size;
  size.width  = width;
  size.height = height;
  target_->Resize(size);
}

void RenderTargetD2D::UpdateMonitor()
{
  // Updates rendering parameters according to current monitor.

  HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
  if (monitor != hmonitor_)
  {
    // Create based on monitor settings, rather than the defaults of
    // gamma=1.8, contrast=.5, and clearTypeLevel=.5

    IDWriteRenderingParams* renderingParams = nullptr;

    dwriteFactory_->CreateMonitorRenderingParams(monitor, &renderingParams);
    target_->SetTextRenderingParams(renderingParams);

    hmonitor_ = monitor;
    InvalidateRect(hwnd_, nullptr, FALSE);

    SafeRelease(&renderingParams);
  }
}

void RenderTargetD2D::BeginDraw()
{
  target_->BeginDraw();
  target_->SetTransform(D2D1::Matrix3x2F::Identity());
}

void RenderTargetD2D::EndDraw()
{
  HRESULT hr = target_->EndDraw();

  // If the device is lost for any reason, we need to recreate it.
  if (hr == D2DERR_RECREATE_TARGET)
  {
    // Flush resources and recreate them.
    // This is very rare for a device to be lost,
    // but it can occur when connecting via Remote Desktop.
    // imageCache_.Clear();
    hmonitor_ = nullptr;

    CreateTarget();
  }
}

void RenderTargetD2D::Clear(UINT32 color)
{
  target_->Clear(D2D1::ColorF(color));
}

#if 0
ID2D1Bitmap* RenderTargetD2D::GetCachedImage(IWICBitmapSource* image)
{
    // Maps a WIC image source to an aready cached D2D bitmap.
    // If not already cached, it creates the D2D bitmap from WIC.

    if (image == nullptr)
        return nullptr;

    // Find an existing match
    mj::ArrayList<ImageCacheEntry>::iterator match = std::find(imageCache_.begin(), imageCache_.end(), image);
    if (match != imageCache_.end())
        return match->converted; // already cached

    // Convert the WIC image to a ready-to-use device-dependent D2D bitmap.
    // This avoids needing to recreate a new texture every draw call, but
    // allows easy reconstruction of textures if the device changes and
    // resources need recreation (also lets callers be D2D agnostic).

    ID2D1Bitmap* bitmap = nullptr;
    target_->CreateBitmapFromWicBitmap(image, nullptr, &bitmap);
    if (bitmap == nullptr)
        return nullptr;

    // Save for later calls.
        imageCache_.push_back(ImageCacheEntry(image, bitmap));

    // Release it locally and return the pointer.
    // The bitmap is now referenced by the bitmap cache.
    bitmap->Release();
    return bitmap;
}
#endif

void RenderTargetD2D::FillRectangle(const RectF& destRect, const DrawingEffect& drawingEffect)
{
  ID2D1Brush* brush = GetCachedBrush(&drawingEffect);
  if (brush == nullptr)
    return;

  // We will always get a strikethrough as a LTR rectangle
  // with the baseline origin snapped.
  target_->FillRectangle(destRect, brush);
}

#if 0
void RenderTargetD2D::DrawImage(
    IWICBitmapSource* image,
    const RectF& sourceRect,  // where in source atlas texture
    const RectF& destRect     // where on display to draw it
    )
{
    // Ignore zero size source rects.
    // Draw nothing if the destination is zero size.
    if (&sourceRect    == nullptr
    || sourceRect.left >= sourceRect.right
    || sourceRect.top  >= sourceRect.bottom
    || destRect.left   >= destRect.right
    || destRect.top    >= destRect.bottom)
    {
        return;
    }

    ID2D1Bitmap* bitmap = GetCachedImage(image);
    if (bitmap == nullptr)
        return;

    target_->DrawBitmap(
            bitmap,
            destRect,
            1.0, // opacity
            D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
            sourceRect
            );
}
#endif

void RenderTargetD2D::DrawTextLayout(IDWriteTextLayout* textLayout, const RectF& rect)
{
  if (textLayout == nullptr)
    return;

  Context context(this, nullptr);
  textLayout->Draw(&context, this, rect.left, rect.top);
}

ID2D1Brush* RenderTargetD2D::GetCachedBrush(const DrawingEffect* effect)
{
  if (effect == nullptr || brush_ == nullptr)
    return nullptr;

  // Update the D2D brush to the new effect color.
  UINT32 bgra = effect->GetColor();
  float alpha = (bgra >> 24) / 255.0f;
  brush_->SetColor(D2D1::ColorF(bgra, alpha));

  return brush_;
}

void RenderTargetD2D::SetTransform(DWRITE_MATRIX const& transform)
{
  target_->SetTransform(reinterpret_cast<const D2D1_MATRIX_3X2_F*>(&transform));
}

void RenderTargetD2D::GetTransform(DWRITE_MATRIX& transform)
{
  target_->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(&transform));
}

void RenderTargetD2D::SetAntialiasing(bool isEnabled)
{
  target_->SetAntialiasMode(isEnabled ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::DrawGlyphRun(void* clientDrawingContext, FLOAT baselineOriginX,
                                                        FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode,
                                                        const DWRITE_GLYPH_RUN* glyphRun,
                                                        const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
                                                        IUnknown* clientDrawingEffect)
{
  // If no drawing effect is applied to run, but a clientDrawingContext
  // is passed, use the one from that instead. This is useful for trimming
  // signs, where they don't have a color of their own.
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  // Since we use our own custom renderer and explicitly set the effect
  // on the layout, we know exactly what the parameter is and can
  // safely cast it directly.
  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (brush == nullptr)
    return E_FAIL;

  target_->DrawGlyphRun(D2D1::Point2(baselineOriginX, baselineOriginY), glyphRun, brush, measuringMode);

  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::DrawUnderline(void* clientDrawingContext, FLOAT baselineOriginX,
                                                         FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline,
                                                         IUnknown* clientDrawingEffect)
{
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (brush == nullptr)
    return E_FAIL;

  // We will always get a strikethrough as a LTR rectangle
  // with the baseline origin snapped.
  D2D1_RECT_F rectangle = { baselineOriginX, baselineOriginY + underline->offset, baselineOriginX + underline->width,
                            baselineOriginY + underline->offset + underline->thickness };

  // Draw this as a rectangle, rather than a line.
  target_->FillRectangle(&rectangle, brush);

  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::DrawStrikethrough(void* clientDrawingContext, FLOAT baselineOriginX,
                                                             FLOAT baselineOriginY,
                                                             const DWRITE_STRIKETHROUGH* strikethrough,
                                                             IUnknown* clientDrawingEffect)
{
  clientDrawingEffect = GetDrawingEffect(clientDrawingContext, clientDrawingEffect);

  DrawingEffect* effect = static_cast<DrawingEffect*>(clientDrawingEffect);
  ID2D1Brush* brush     = GetCachedBrush(effect);
  if (brush == nullptr)
    return E_FAIL;

  // We will always get an underline as a LTR rectangle
  // with the baseline origin snapped.
  D2D1_RECT_F rectangle = { baselineOriginX, baselineOriginY + strikethrough->offset,
                            baselineOriginX + strikethrough->width,
                            baselineOriginY + strikethrough->offset + strikethrough->thickness };

  // Draw this as a rectangle, rather than a line.
  target_->FillRectangle(&rectangle, brush);

  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::DrawInlineObject(void* clientDrawingContext, FLOAT originX, FLOAT originY,
                                                            IDWriteInlineObject* inlineObject, BOOL isSideways,
                                                            BOOL isRightToLeft, IUnknown* clientDrawingEffect)
{
  // Inline objects inherit the drawing effect of the text
  // they are in, so we should pass it down (if none is set
  // on this range, use the drawing context's effect instead).
  Context subContext(*reinterpret_cast<RenderTarget::Context*>(clientDrawingContext));

  if (clientDrawingEffect != nullptr)
    subContext.drawingEffect = clientDrawingEffect;

  inlineObject->Draw(&subContext, this, originX, originY, false, false, subContext.drawingEffect);

  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::IsPixelSnappingDisabled(void* clientDrawingContext, OUT BOOL* isDisabled)
{
  // Enable pixel snapping of the text baselines,
  // since we're not animating and don't want blurry text.
  *isDisabled = FALSE;
  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::GetCurrentTransform(void* clientDrawingContext, OUT DWRITE_MATRIX* transform)
{
  // Simply forward what the real renderer holds onto.
  target_->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F*>(transform));
  return S_OK;
}

HRESULT STDMETHODCALLTYPE RenderTargetD2D::GetPixelsPerDip(void* clientDrawingContext, OUT FLOAT* pixelsPerDip)
{
  // Any scaling will be combined into matrix transforms rather than an
  // additional DPI scaling. This simplifies the logic for rendering
  // and hit-testing. If an application does not use matrices, then
  // using the scaling factor directly is simpler.
  *pixelsPerDip = 1;
  return S_OK;
}
