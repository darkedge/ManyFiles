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
#pragma once

struct ID2D1HwndRenderTarget;
struct ID2D1Bitmap;
class InlineImage;
class DrawingEffect;
typedef D2D1_RECT_F RectF;

////////////////////////////////////////////////////////////////////////////////

class RenderTarget;

// Intermediate render target for UI to draw to either a D2D or GDI surface.
class DECLSPEC_UUID("4327AC14-3172-4807-BF40-02C7475A2520") RenderTarget
    : public ComBase<QiListSelf<RenderTarget, QiList<IDWriteTextRenderer>>>
{
public:
  virtual ~RenderTarget(){};

  virtual void BeginDraw()                     = 0;
  virtual void EndDraw()                       = 0;
  virtual void Clear(UINT32 color)             = 0;
  virtual void Resize(UINT width, UINT height) = 0;
  virtual void UpdateMonitor()                 = 0;

  virtual void SetTransform(DWRITE_MATRIX const& transform) = 0;
  virtual void GetTransform(DWRITE_MATRIX& transform)       = 0;
  virtual void SetAntialiasing(bool isEnabled)              = 0;

  virtual void DrawTextLayout(IDWriteTextLayout* textLayout, const RectF& rect) = 0;

  virtual void FillRectangle(const RectF& destRect, const DrawingEffect& drawingEffect) = 0;

protected:
  // This context is not persisted, only existing on the stack as it
  // is passed down through. This is mainly needed to handle cases
  // where runs where no drawing effect set, like those of an inline
  // object or trimming sign.
  struct Context
  {
    Context(RenderTarget* initialTarget, IUnknown* initialDrawingEffect)
        : target(initialTarget), drawingEffect(initialDrawingEffect)
    {
    }

    // short lived weak pointers
    RenderTarget* target;
    IUnknown* drawingEffect;
  };

  IUnknown* GetDrawingEffect(void* clientDrawingContext, IUnknown* drawingEffect)
  {
    // Callbacks use this to use a drawing effect from the client context
    // if none was passed into the callback.
    if (drawingEffect != nullptr)
      return drawingEffect;

    return (reinterpret_cast<Context*>(clientDrawingContext))->drawingEffect;
  }
};

////////////////////////////////////////////////////////////////////////////////

class RenderTargetD2D : public RenderTarget
{
public:
  RenderTargetD2D(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd);
  HRESULT static Create(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd,
                        OUT RenderTarget** renderTarget);

  virtual ~RenderTargetD2D();

  virtual void BeginDraw();
  virtual void EndDraw();
  virtual void Clear(UINT32 color);
  virtual void Resize(UINT width, UINT height);
  virtual void UpdateMonitor();

  virtual void SetTransform(DWRITE_MATRIX const& transform);
  virtual void GetTransform(DWRITE_MATRIX& transform);
  virtual void SetAntialiasing(bool isEnabled);

  virtual void DrawTextLayout(IDWriteTextLayout* textLayout, const RectF& rect);

  void FillRectangle(const RectF& destRect, const DrawingEffect& drawingEffect);

  // IDWriteTextRenderer implementation

  IFACEMETHOD(DrawGlyphRun)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode,
   const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
   IUnknown* clientDrawingEffect);

  IFACEMETHOD(DrawUnderline)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline,
   IUnknown* clientDrawingEffect);

  IFACEMETHOD(DrawStrikethrough)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough,
   IUnknown* clientDrawingEffect);

  IFACEMETHOD(DrawInlineObject)
  (void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways,
   BOOL isRightToLeft, IUnknown* clientDrawingEffect);

  IFACEMETHOD(IsPixelSnappingDisabled)(void* clientDrawingContext, OUT BOOL* isDisabled);

  IFACEMETHOD(GetCurrentTransform)(void* clientDrawingContext, OUT DWRITE_MATRIX* transform);

  IFACEMETHOD(GetPixelsPerDip)(void* clientDrawingContext, OUT FLOAT* pixelsPerDip);

protected:
  HRESULT CreateTarget();
  ID2D1Brush* GetCachedBrush(const DrawingEffect* effect);

protected:
  IDWriteFactory* dwriteFactory_;
  ID2D1Factory* d2dFactory_;
  ID2D1HwndRenderTarget* target_; // D2D render target
  ID2D1SolidColorBrush* brush_;   // reusable scratch brush for current color

  HWND hwnd_;
  HMONITOR hmonitor_;
};
