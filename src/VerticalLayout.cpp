#include "VerticalLayout.h"
#include "ResourcesD2D1.h"

static constexpr const int16_t s_ResizeControlHeight = 8;

void mj::VerticalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
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

void mj::VerticalLayout::MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t dx,
                                           int16_t dy)
{
  static_cast<void>(dx);

  pFirst->height += dy;
  pResizeControl->yParent += dy;
  pSecond->yParent += dy;
  pSecond->height -= dy;
}

void mj::VerticalLayout::Destroy()
{
  // We currently do not own the controls,
  // So we leave individual control destruction up to the owner.

  // Of course, we still have to clean up our own resources
  // TODO: Destroy all HorizontalResizeControl (which we own)
  this->controls.Destroy();

  // We don't own the allocator, so just remove the reference.
  this->pAllocator = nullptr;
}

void mj::VerticalLayout::OnSize()
{
  if (this->controls.Size() > 0)
  {
    int16_t numResizeControls         = this->controls.Size() - 1;
    int16_t totalResizeControlsHeight = numResizeControls * s_ResizeControlHeight;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t controlHeight = (this->height - totalResizeControlsHeight) / this->controls.Size();
    int16_t y             = 0;

    for (size_t i = 0; i < this->controls.Size(); i++)
    {
      Control* pControl = this->controls[i];

      pControl->xParent = 0;
      pControl->yParent = y;
      pControl->width   = width;
      pControl->height  = controlHeight;
      pControl->OnSize();
      y += controlHeight;

      if ((i + 1) < this->controls.Size())
      {
        y += s_ResizeControlHeight;
      }
    }
  }
}

void mj::VerticalLayout::Add(mj::Control* pControl)
{
  // TODO: Add HorizontalResizeControl
  this->controls.Add(pControl);
}
