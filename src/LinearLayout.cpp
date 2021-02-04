#include "LinearLayout.h"
#include "ResourcesD2D1.h"
#include "ServiceLocator.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::LinearLayout::Init(AllocatorBase* pAllocator)
{
  this->pAllocator = pAllocator;
  this->controls.Init(pAllocator);
}

void mj::LinearLayout::OnMouseMove(int16_t x, int16_t y)
{
  if (this->resizeControlIndex != 0)
  {
    Control* pFirst = this->controls[this->resizeControlIndex - 1];
    Control* pResizeControl = this->controls[this->resizeControlIndex];
    Control* pSecond = this->controls[this->resizeControlIndex + 1];
    
    int16_t dx = x - this->dragStartX;
    int16_t dy = y - this->dragStartY;

    this->MoveResizeControl(pFirst, pResizeControl, pSecond, dx, dy);

    this->dragStartX = x;
    this->dragStartY = y;

    ::InvalidateRect(svc::MainWindowHandle(), nullptr, false);
  }
  else
  {
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
}

bool mj::LinearLayout::OnLeftButtonDown(int16_t x, int16_t y)
{
  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];

    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      if (pControl->OnLeftButtonDown(clientX, clientY))
      {
        if (i & 1)
        {
          // Start resizing
          this->resizeControlIndex = i;
          this->dragStartX = x;
          this->dragStartY = y;
        }
        return true;
      }
      break;
    }
  }

  return false;
}

bool mj::LinearLayout::OnLeftButtonUp(int16_t x, int16_t y)
{
  // Stop resizing
  this->resizeControlIndex = 0;

  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];

    int16_t clientX = x;
    int16_t clientY = y;
    if (pControl->TranslateClientPoint(&clientX, &clientY))
    {
      if (pControl->OnLeftButtonUp(clientX, clientY))
      {
        return true;
      }
      break;
    }
  }

  return false;
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
