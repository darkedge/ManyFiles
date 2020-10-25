﻿#pragma once
#include "mj_win32.h"

struct ID2D1HwndRenderTarget;
struct ID2D1Bitmap;
class InlineImage;
class DrawingEffect;
typedef D2D1_RECT_F RectF;

class RenderTarget : public IDWriteTextRenderer
{
public:
  RenderTarget(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd);
  HRESULT static Create(ID2D1Factory* d2dFactory, IDWriteFactory* dwriteFactory, HWND hwnd,
                        OUT RenderTarget** renderTarget);

  ~RenderTarget();

  void BeginDraw();
  void EndDraw();
  void Clear(UINT32 color);
  void Resize(UINT width, UINT height);
  void UpdateMonitor();

  void SetTransform(DWRITE_MATRIX const& transform);
  void GetTransform(DWRITE_MATRIX& transform);
  void SetAntialiasing(bool isEnabled);

  void DrawTextLayout(IDWriteTextLayout* textLayout, const RectF& rect);

  void FillRectangle(const RectF& destRect, const DrawingEffect& drawingEffect);

  // IDWriteTextRenderer interface implementation
  IFACEMETHOD(DrawGlyphRun)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, DWRITE_MEASURING_MODE measuringMode,
   const DWRITE_GLYPH_RUN* glyphRun, const DWRITE_GLYPH_RUN_DESCRIPTION* glyphRunDescription,
   IUnknown* clientDrawingEffect) override;

  IFACEMETHOD(DrawUnderline)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_UNDERLINE* underline,
   IUnknown* clientDrawingEffect) override;

  IFACEMETHOD(DrawStrikethrough)
  (void* clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH* strikethrough,
   IUnknown* clientDrawingEffect) override;

  IFACEMETHOD(DrawInlineObject)
  (void* clientDrawingContext, FLOAT originX, FLOAT originY, IDWriteInlineObject* inlineObject, BOOL isSideways,
   BOOL isRightToLeft, IUnknown* clientDrawingEffect) override;

  // IDWriteTextRenderer -> IDWritePixelSnapping interface implementation
  IFACEMETHOD(IsPixelSnappingDisabled)(void* clientDrawingContext, OUT BOOL* isDisabled);

  IFACEMETHOD(GetCurrentTransform)(void* clientDrawingContext, OUT DWRITE_MATRIX* transform);

  IFACEMETHOD(GetPixelsPerDip)(void* clientDrawingContext, OUT FLOAT* pixelsPerDip);

  // IDWriteTextRenderer -> IDWritePixelSnapping -> IUnknown interface implementation
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(
      /* [in] */ REFIID riid,
      /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
  {
    return mj::ENotImpl;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef() override
  {
    return 0;
  }

  virtual ULONG STDMETHODCALLTYPE Release() override
  {
    return 0;
  }

private:
  HRESULT CreateTarget();
  ID2D1Brush* GetCachedBrush(const DrawingEffect* effect);

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
    if (drawingEffect)
      return drawingEffect;

    return (reinterpret_cast<Context*>(clientDrawingContext))->drawingEffect;
  }

private:
  mj::ComPtr<IDWriteFactory> dwriteFactory_;
  mj::ComPtr<ID2D1Factory> d2dFactory_;
  mj::ComPtr<ID2D1HwndRenderTarget> target_; // D2D render target
  mj::ComPtr<ID2D1SolidColorBrush> brush_;   // reusable scratch brush for current color

  HWND hwnd_;
  HMONITOR hmonitor_;
};
