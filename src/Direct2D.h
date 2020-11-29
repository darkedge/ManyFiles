#pragma once
#include <d2d1.h>
#include <dwrite.h>

namespace mj
{
  void Direct2DInit(HWND hwnd);
  void RenderHexEditBuffer();
  UINT GetRenderedTextHeight();
  void Direct2DDraw(int y);
} // namespace mj
