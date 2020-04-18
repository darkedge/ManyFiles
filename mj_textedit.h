#pragma once
#include <dwrite.h>
#include <d2d1.h>
#include "mj_gapbuffer.h"

namespace mj
{
  struct TextEditLine
  {
    wchar_t* pText    = nullptr;
    size_t textLength = 0;
    IDWriteTextLayout* pTextLayout = nullptr;
  };

  struct TextEdit
  {
    void* pMemory;
    TextEditLine* pLines;
    mj::GapBuffer buf;
  };

  HRESULT TextEditCreateDeviceResources(TextEdit* pTextEdit, IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat,
                                        float width, float height);
  void TextEditOnClick(TextEdit* pTextEdit, UINT x, UINT y);
  HRESULT TextEditInit(TextEdit* pTextEdit);
  void TextEditWndProc(TextEdit* pTextEdit, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  void TextEditDraw(TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget, ID2D1SolidColorBrush* pBrush);
  void TextEditDestroy(TextEdit* pTextEdit);
} // namespace mj