#include "LinearLayout.h"
#include "ResourcesD2D1.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::LinearLayout::Init(AllocatorBase* pAllocator)
{
  this->controls.Init(pAllocator);
}

void mj::LinearLayout::Destroy()
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

void mj::LinearLayout::OnMouseMove(int16_t x, int16_t y)
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

void mj::LinearLayout::OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
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

void mj::LinearLayout::OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta)
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

void mj::LinearLayout::OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY)
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

void mj::LinearLayout::Add(mj::Control* pControl)
{
  this->controls.Add(pControl);
}
