#pragma once
#include <wrl/client.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <d2d1.h>
#include "mj_gapbuffer.h"

namespace mj
{
  struct TextEdit
  {
    void* pMemory;
    std::vector<Microsoft::WRL::ComPtr<IDWriteTextLayout>> lines;
    std::wstring line;
    mj::GapBuffer buf;
  };

  HRESULT TextEditCreateDeviceResources(TextEdit* pTextEdit, IDWriteFactory* pFactory, IDWriteTextFormat* pTextFormat, float width, float height);
  void TextEditOnClick(TextEdit* pTextEdit, UINT x, UINT y);
  HRESULT TextEditInit(TextEdit* pTextEdit);
  void TextEditWndProc(TextEdit* pTextEdit, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  void TextEditDraw(TextEdit* pTextEdit, ID2D1HwndRenderTarget* pRenderTarget, ID2D1SolidColorBrush* pBrush);
  void TextEditDestroy(TextEdit* pTextEdit);
} // namespace mj