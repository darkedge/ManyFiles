#pragma once
#include <dwrite.h>
#include <d2d1.h>

namespace mj
{
  struct Direct2D
  {
    IDWriteFactory* pDWriteFactory;
    ID2D1Factory* pDirect2DFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
  };

  bool Direct2DInit(HWND hwnd, Direct2D* pDirect2D);

} // namespace mj
