#include "HorizontalLayout.h"
#include "VerticalResizeControl.h"

void mj::HorizontalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
{
  static_cast<void>(pRenderTarget);

  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];
    pControl->OnPaint(pRenderTarget);
  }
}

void mj::HorizontalLayout::MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t dx,
                                             int16_t dy)
{
  static_cast<void>(dy);

  pFirst->width += dx;
  pResizeControl->xParent += dx;
  pSecond->xParent += dx;
  pSecond->width -= dx;
}

void mj::HorizontalLayout::OnSize()
{
  const int16_t numControls = static_cast<int16_t>(this->controls.Size());

  if (numControls > 0)
  {
    int16_t numResizeControls        = numControls / 2;
    int16_t numPanels                = numResizeControls + 1;
    int16_t totalResizeControlsWidth = numResizeControls * VerticalResizeControl::WIDTH;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t panelWidth = (this->width - totalResizeControlsWidth) / numPanels;
    int16_t x          = 0;

    for (size_t i = 0; i < numControls; i++)
    {
      Control* pControl = this->controls[i];

      pControl->xParent = x;
      pControl->yParent = 0;
      if (!(i & 1))
      {
        pControl->width = panelWidth;
      }
      pControl->height = this->height;
      pControl->OnSize();

      x += pControl->width;
    }
  }
}

void mj::HorizontalLayout::Add(mj::Control* pControl)
{
  if (this->controls.Size() > 0)
  {
    Control* pResizeControl = this->pAllocator->New<VerticalResizeControl>();
    pResizeControl->Init(this->pAllocator);
    this->controls.Add(pResizeControl);
  }
  this->controls.Add(pControl);
}
