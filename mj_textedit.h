#pragma once
#include <dwrite.h>
#include "mj_gapbuffer.h"
#include "mj_common.h"

struct ID2D1HwndRenderTarget;
struct RenderTargetResources;
struct ID2D1SolidColorBrush;

namespace mj
{
  struct ECursor
  {
    enum Enum
    {
      ARROW,
      IBEAM,
      WAIT,
      CROSS,
      UPARROW,
      SIZENWSE,
      SIZENESW,
      SIZEWE,
      SIZENS,
      SIZEALL,
      NO,
      HAND,
      APPSTARTING,
      HELP,
      PIN,
      PERSON,
    };
  };

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
    [[nodiscard]] HRESULT CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat, const mj::GapBuffer& buffer,
                                  FLOAT width, FLOAT height);
    [[nodiscard]] FLOAT GetWidth() const;
    [[nodiscard]] bool MouseDown(SHORT x, SHORT y, UINT32& textPosition);
  };

  enum class EDraggable
  {
    NONE,
    TEXT_SELECT,
    TEXT_MOVE,
    HOR_SCROLLBAR,
    VER_SCROLLBAR
  };

  struct DragAction
  {
    EDraggable draggable;
    FLOAT start;
    SHORT mouseStartX;
    SHORT mouseStartY;
  };

  class TextEdit;
  class HorizontalScrollBar
  {
  private:
    D2D1_RECT_F back;
    D2D1_RECT_F front;
    TextEdit* pParent;

  public:
    void Init(TextEdit* pParent);
    void Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources);
    [[nodiscard]] bool MouseDown(SHORT x, SHORT y);
  };

  class TextEdit
  {
  private:
    void* pMemory;
    TextView text;
    mj::GapBuffer buf;
    D2D1_RECT_F widgetRect; // Rect of widget inside rendertarget
    FLOAT width;            // Equal to width of the longest rendered line
    DragAction drag;
    D2D1_POINT_2F scrollAmount; // Position of scroll area
    HorizontalScrollBar horizontalScrollBar;
    FLOAT margin;

  public:
    [[nodiscard]] HRESULT CreateDeviceResources(IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat, FLOAT width, FLOAT height);
    void MouseDown(SHORT x, SHORT y);
    void MouseUp();
    [[nodiscard]] ECursor::Enum MouseMove(SHORT x, SHORT y);
    [[nodiscard]] HRESULT Init(FLOAT margin, FLOAT parentWidth, FLOAT parentHeight);
    void Resize(FLOAT width, FLOAT height);
    void WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    void Draw(ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources);
    void Destroy();

    MJ_CRGETTER(GetDragAction, drag);
    MJ_CRGETTER(GetWidgetRect, widgetRect);
    MJ_GETTER(GetWidth, width);
    MJ_CRGETTER(GetScrollPosition, scrollAmount);
  };
} // namespace mj
