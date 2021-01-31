#include "HorizontalLayout.h"
#include "ResourcesD2D1.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::HorizontalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
{
  auto pBrush = res::d2d1::ResizeControlBrush();
  if (pBrush)
  {
    // pRenderTarget->FillRectangle(D2D1::RectF(x, 0, x2, this->height), pBrush);
  }

  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];
    pControl->OnPaint();
  }
}

void mj::HorizontalLayout::OnSize()
{
  if (this->controls.Size() > 0)
  {
    int16_t numResizeControls        = this->controls.Size() - 1;
    int16_t totalResizeControlsWidth = numResizeControls * s_ResizeControlWidth;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t controlWidth = (this->width - totalResizeControlsWidth) / this->controls.Size();
    int16_t x            = 0;

    for (size_t i = 0; i < this->controls.Size(); i++)
    {
      Control* pControl = this->controls[i];

      pControl->xParent = x;
      pControl->yParent = 0;
      pControl->width   = controlWidth;
      pControl->height  = height;
      pControl->OnSize();
      x += controlWidth;

      if ((i + 1) < this->controls.Size())
      {
        x += s_ResizeControlWidth;
      }
    }
  }
}
