#pragma once
#include <dwrite.h>

struct ID2D1HwndRenderTarget;
struct RenderTargetResources;
struct ID2D1SolidColorBrush;

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
        ID2D1HwndRenderTarget* pRenderTarget;
        RenderTargetResources* pResources;
      };

      virtual HRESULT DrawGlyphRun(void* clientDrawingContext,                              //
                                   FLOAT baselineOriginX,                                   //
                                   FLOAT baselineOriginY,                                   //
                                   DWRITE_MEASURING_MODE measuringMode,                     //
                                   DWRITE_GLYPH_RUN const* glyphRun,                        //
                                   DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription, //
                                   IUnknown* clientDrawingEffect) override;

      virtual HRESULT DrawUnderline(void* clientDrawingContext,        //
                                    FLOAT baselineOriginX,             //
                                    FLOAT baselineOriginY,             //
                                    DWRITE_UNDERLINE const* underline, //
                                    IUnknown* clientDrawingEffect) override;

      virtual HRESULT DrawStrikethrough(void* clientDrawingContext,                //
                                        FLOAT baselineOriginX,                     //
                                        FLOAT baselineOriginY,                     //
                                        DWRITE_STRIKETHROUGH const* strikethrough, //

                                        IUnknown* clientDrawingEffect) override;
      virtual HRESULT DrawInlineObject(void* clientDrawingContext,        //
                                       FLOAT originX,                     //
                                       FLOAT originY,                     //
                                       IDWriteInlineObject* inlineObject, //
                                       BOOL isSideways,                   //
                                       BOOL isRightToLeft,                //
                                       IUnknown* clientDrawingEffect) override;

      // IDWritePixelSnapping
      virtual HRESULT IsPixelSnappingDisabled(void* clientDrawingContext, BOOL* isDisabled) override;
      virtual HRESULT GetCurrentTransform(void* clientDrawingContext, DWRITE_MATRIX* transform) override;
      virtual HRESULT GetPixelsPerDip(void* clientDrawingContext, FLOAT* pixelsPerDip) override;

      // IUnknown
      virtual HRESULT QueryInterface(REFIID riid, void** ppvObject) override;
      virtual ULONG AddRef() override;
      virtual ULONG Release() override;
    };

    wchar_t* pText;
    size_t textLength;
    IDWriteTextLayout* pTextLayout;
    alignas(TextViewRenderer) char renderer_storage[sizeof(TextViewRenderer)];
    TextViewRenderer* pRenderer;
    UINT32 hoverPosition;

  public:
    void Init();
    void Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources, UINT32 textPosition);
    void MouseMove(SHORT x, SHORT y);
    [[nodiscard]] HRESULT CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                                const mj::GapBuffer& buffer, FLOAT width, FLOAT height);
    [[nodiscard]] FLOAT GetWidth() const;
    [[nodiscard]] bool GetTextPosition(SHORT x, SHORT y, UINT32& textPosition);
  };
} // namespace mj
