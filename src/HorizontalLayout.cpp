#include "HorizontalLayout.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::HorizontalLayout::Init(AllocatorBase* pAllocator)
{
  this->controls.Init(pAllocator);
}

void mj::HorizontalLayout::OnPaint()
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
#if 0
  for (int32_t i = 0; i < 2; i++)
  {
    Control* pControl = this->controls[i];
    if (pControl)
    {
      pControl->Destroy();
      this->pAllocator->Free(pControl);
      this->controls[i] = nullptr;
    }
  }
#endif

  // Of course, we still have to clean up our own resources
  this->controls.Destroy();
}

void mj::HorizontalLayout::OnSize()
{
  if (this->controls.Size() > 0)
  {
    int16_t numResizeControls = this->controls.Size() - 1;
    
    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t controlWidth = this->width / this->controls.Size();
    int16_t x            = 0;

    for (Control* pControl : this->controls)
    {
      pControl->x      = x;
      pControl->y      = 0;
      pControl->width  = controlWidth;
      pControl->height = height;
      pControl->OnSize();
      x += controlWidth;
    }
  }
}

void mj::HorizontalLayout::OnMouseMove(int16_t x, int16_t y)
{
  // TODO: If resizable and dragging: resize two controls
  for (Control* pControl : this->controls)
  {
    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      pControl->OnMouseMove(clientX, clientY);
      break;
    }
  }
}

void mj::HorizontalLayout::OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
{
  for (Control* pControl : this->controls)
  {
    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      pControl->OnDoubleClick(clientX, clientY, mkMask);
      break;
    }
  }
}

void mj::HorizontalLayout::OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta)
{
  for (Control* pControl : this->controls)
  {
    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      pControl->OnMouseWheel(x, y, mkMask, zDelta);
      break;
    }
  }
}

void mj::HorizontalLayout::OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY)
{
  for (Control* pControl : this->controls)
  {
    int16_t x = clientX;
    int16_t y = clientY;
    if (pControl->TranslateClientPoint(&x, &y))
    {
      pControl->OnContextMenu(x, y, screenX, screenY);
      break;
    }
  }
}

void mj::HorizontalLayout::Add(mj::Control* pControl)
{
  this->controls.Add(pControl);
}
