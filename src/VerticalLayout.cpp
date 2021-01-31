#include "VerticalLayout.h"
#include "ResourcesD2D1.h"

static constexpr const int16_t s_ResizeControlHeight = 8;

void mj::VerticalLayout::Init(AllocatorBase* pAllocator)
{
  this->controls.Init(pAllocator);
}

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

void mj::VerticalLayout::Destroy()
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

void mj::VerticalLayout::OnSize()
{
  if (this->controls.Size() > 0)
  {
    int16_t numResizeControls        = this->controls.Size() - 1;
    int16_t totalResizeControlsHeight = numResizeControls * s_ResizeControlHeight;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t controlHeight = (this->height - totalResizeControlsHeight) / this->controls.Size();
    int16_t y            = 0;

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

void mj::VerticalLayout::OnMouseMove(int16_t x, int16_t y)
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

void mj::VerticalLayout::OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
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

void mj::VerticalLayout::OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta)
{
  for (Control* pControl : this->controls)
  {
    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      pControl->OnMouseWheel(clientX, clientY, mkMask, zDelta);
      break;
    }
  }
}

void mj::VerticalLayout::OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY)
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

void mj::VerticalLayout::Add(mj::Control* pControl)
{
  this->controls.Add(pControl);
}
