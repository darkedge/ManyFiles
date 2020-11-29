#pragma once
#include <d3d11_1.h>
#include <d2d1_1.h>
#include <dwrite.h>

namespace mj
{
  void Direct2DInit(HWND hwnd);
  void RenderHexEditBuffer();
  UINT GetRenderedTextHeight();
  void Direct2DDraw(int y);
  void Direct2DOnSize(WORD width, WORD height);
  void Direct2DDestroy();
} // namespace mj
