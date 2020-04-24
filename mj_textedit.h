#pragma once
#include <dwrite.h>
#include "mj_gapbuffer.h"

struct ID2D1HwndRenderTarget;
struct RenderTargetResources;

namespace mj
{
  enum class ECursor
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

  struct TextEditLine
  {
    wchar_t* pText                 = nullptr;
    size_t textLength              = 0;
    IDWriteTextLayout* pTextLayout = nullptr;
  };

  enum class EDraggable
  {
    NONE,
    TEXT_SELECT,
    TEXT_MOVE,
    HOR_SCROLLBAR,
    VER_SCROLLBAR
  };

  struct Drag
  {
    EDraggable draggable;
    FLOAT start;
    SHORT mouseStartX;
    SHORT mouseStartY;
  };

  struct TextEdit
  {
    struct Reverse
    {
      RECT horScrollbarRect;
    };

    void* pMemory;
    TextEditLine* pLines;
    mj::GapBuffer buf;
    D2D1_RECT_F widgetRect;  // Rect of widget inside rendertarget
    D2D1_POINT_2F scrollPos; // Position of scroll area
    FLOAT width;             // Equal to width of the longest rendered line
    Drag drag;
    Reverse reverse;
  };

  HRESULT TextEditCreateDeviceResources(TextEdit* pTextEdit, IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                        FLOAT width, FLOAT height);
  void TextEditMouseDown(TextEdit* pTextEdit, SHORT x, SHORT y);
  void TextEditMouseUp(TextEdit* pTextEdit, SHORT x, SHORT y);
  ECursor TextEditMouseMove(TextEdit* pTextEdit, SHORT x, SHORT y);
  HRESULT TextEditInit(TextEdit* pTextEdit, FLOAT left, FLOAT top, FLOAT right, FLOAT bottom);
  void TextEditWndProc(TextEdit* pTextEdit, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  void TextEditDraw(TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources);
  void TextEditDestroy(TextEdit* pTextEdit);
} // namespace mj
