#pragma once
#include <dwrite.h>
#include "mj_gapbuffer.h"

struct ID2D1HwndRenderTarget;
struct RenderTargetResources;

namespace mj
{
  struct TextEditLine
  {
    wchar_t* pText                 = nullptr;
    size_t textLength              = 0;
    IDWriteTextLayout* pTextLayout = nullptr;
  };

  struct TextEdit
  {
    void* pMemory;
    TextEditLine* pLines;
    mj::GapBuffer buf;
    D2D1_RECT_F widgetRect; // Rect of widget inside rendertarget
    D2D1_POINT_2F scrollPos; // Position of scroll area
    FLOAT width; // Equal to width of the longest rendered line
  };

  HRESULT TextEditCreateDeviceResources(TextEdit* pTextEdit, IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                        float width, float height);
  void TextEditOnClick(TextEdit* pTextEdit, UINT x, UINT y);
  HRESULT TextEditInit(TextEdit* pTextEdit, FLOAT left, FLOAT top, FLOAT right, FLOAT bottom);
  void TextEditWndProc(TextEdit* pTextEdit, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  void TextEditDraw(TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget, RenderTargetResources* pResources);
  void TextEditDestroy(TextEdit* pTextEdit);
} // namespace mj
