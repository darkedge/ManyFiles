#pragma once
#include <dwrite.h>
#include "mj_win32.h"

struct ID2D1HwndRenderTarget;
struct RenderTargetResources;
struct ID2D1SolidColorBrush;
struct ID2D1RenderTarget;

namespace mj
{
  class GapBuffer;
  class TextView
  {
  private:
    class TextViewRenderer : public IDWriteTextRenderer
    {
    public:
      struct DrawingContext
      {
        mj::ComPtr<ID2D1RenderTarget> pRenderTarget;
        RenderTargetResources* pResources;
      };

      // IDWriteTextRenderer
      virtual HRESULT DrawGlyphRun(void* clientDrawingContext,                              //
                                   FLOAT baselineOriginX,                                   //
                                   FLOAT baselineOriginY,                                   //
                                   DWRITE_MEASURING_MODE measuringMode,                     //
                                   DWRITE_GLYPH_RUN const* glyphRun,                        //
                                   DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription, //
                                   IUnknown* clientDrawingEffect) noexcept override;

      virtual HRESULT DrawUnderline(void* clientDrawingContext,        //
                                    FLOAT baselineOriginX,             //
                                    FLOAT baselineOriginY,             //
                                    DWRITE_UNDERLINE const* underline, //
                                    IUnknown* clientDrawingEffect) noexcept override;

      virtual HRESULT DrawStrikethrough(void* clientDrawingContext,                //
                                        FLOAT baselineOriginX,                     //
                                        FLOAT baselineOriginY,                     //
                                        DWRITE_STRIKETHROUGH const* strikethrough, //
                                        IUnknown* clientDrawingEffect) noexcept override;

      virtual HRESULT DrawInlineObject(void* clientDrawingContext,        //
                                       FLOAT originX,                     //
                                       FLOAT originY,                     //
                                       IDWriteInlineObject* inlineObject, //
                                       BOOL isSideways,                   //
                                       BOOL isRightToLeft,                //
                                       IUnknown* clientDrawingEffect) noexcept override;

      // IDWritePixelSnapping
      virtual HRESULT IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) noexcept override;
      virtual HRESULT GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform) noexcept override;
      virtual HRESULT GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) noexcept override;

      // IUnknown
      virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) noexcept override;
      virtual ULONG AddRef() noexcept override;
      virtual ULONG Release() noexcept override;
    };

    wchar_t* pText;
    size_t textLength;
    mj::ComPtr<IDWriteTextLayout> pTextLayout;
    alignas(TextViewRenderer) char renderer_storage[sizeof(TextViewRenderer)];
    TextViewRenderer* pRenderer;
    UINT32 hoverPosition;

  public:
    void Init() noexcept;
    void Draw(mj::ComPtr<ID2D1RenderTarget> pRenderTarget, RenderTargetResources* pResources,
              UINT32 textPosition) noexcept;
    void MouseMove(SHORT x, SHORT y) noexcept;
    [[nodiscard]] HRESULT CreateDeviceResources(mj::ComPtr<IDWriteFactory> pFactory,
                                                mj::ComPtr<IDWriteTextFormat> pTextFormat, const mj::GapBuffer& buffer,
                                                FLOAT width, FLOAT height) noexcept;
    [[nodiscard]] FLOAT GetWidth() const noexcept;
    [[nodiscard]] bool GetTextPosition(SHORT x, SHORT y, UINT32& textPosition) noexcept;
  };
} // namespace mj
