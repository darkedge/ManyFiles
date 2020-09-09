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
    wchar_t* pText;
    size_t textLength;
    IDWriteTextLayout* pTextLayout;
    ID2D1SolidColorBrush* pTextBrush;

  public:
    void Init();
    void Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources, UINT32 textPosition);
    [[nodiscard]] HRESULT CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                                const mj::GapBuffer& buffer, FLOAT width, FLOAT height);
    [[nodiscard]] FLOAT GetWidth() const;
    [[nodiscard]] bool MouseDown(SHORT x, SHORT y, UINT32& textPosition);
  };
} // namespace mj
