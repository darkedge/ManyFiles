#include "VerticalResizeControl.h"
#include "ResourcesD2D1.h"
#include <d2d1.h>

void mj::VerticalResizeControl::Init(AllocatorBase* pAllocator)
{
  this->height = HEIGHT;
}

void mj::VerticalResizeControl::Paint(ID2D1RenderTarget* pRenderTarget)
{
  auto pBrush = res::d2d1::ResizeControlBrush();
  if (pBrush)
  {
    pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, this->width, this->height), pBrush);
  }
}

void mj::VerticalResizeControl::Destroy()
{
}

bool mj::VerticalResizeControl::OnLeftButtonDown(int16_t x, int16_t y)
{
  return true;
}
