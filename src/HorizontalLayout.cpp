#include "HorizontalLayout.h"
#include "VerticalResizeControl.h"

void mj::HorizontalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
{
  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];
    pControl->OnPaint();
  }
}

void mj::HorizontalLayout::Destroy()
{
  // We currently do not own the controls,
  // So we leave individual control destruction up to the owner.
  // Of course, we still have to clean up our own resources

  // We own all resize controls, so destroy those
  for (size_t i = 1; i < this->controls.Size(); i += 2)
  {
    Control* pControl = this->controls[i];
    pControl->Destroy();
    this->pAllocator->Free(pControl);
    this->controls[i] = nullptr;
  }
  this->controls.Destroy();

  // We don't own the allocator, so just remove the reference.
  this->pAllocator = nullptr;
}

void mj::HorizontalLayout::OnSize()
{
  if (this->controls.Size() > 0)
  {
    int16_t numResizeControls        = this->controls.Size() / 2;
    int16_t numPanels                = numResizeControls + 1;
    int16_t totalResizeControlsWidth = numResizeControls * VerticalResizeControl::WIDTH;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t panelWidth = (this->width - totalResizeControlsWidth) / numPanels;
    int16_t x          = 0;

    for (size_t i = 0; i < this->controls.Size(); i++)
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
    auto* pResizeControl = this->pAllocator->New<VerticalResizeControl>();
    pResizeControl->Init(this->pAllocator);
    this->controls.Add(pResizeControl);
  }
  this->controls.Add(pControl);
}
