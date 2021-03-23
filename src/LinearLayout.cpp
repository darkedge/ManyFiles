#include "LinearLayout.h"
#include "ResourcesD2D1.h"
#include "ServiceLocator.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::LinearLayout::Init(AllocatorBase* pAllocator)
{
  this->pAllocator = pAllocator;
  this->controls.Init(pAllocator);
}

void mj::LinearLayout::OnMouseMove(MouseMoveEvent* pMouseMoveEvent)
{
  for (Control* pControl : this->controls)
  {
    MouseMoveEvent mouseMoveEvent = *pMouseMoveEvent;
    if (pControl->TranslateClientPoint(&mouseMoveEvent.x, &mouseMoveEvent.y))
    {
      pControl->OnMouseMove(&mouseMoveEvent);
      *pMouseMoveEvent = mouseMoveEvent;
      break;
    }
  }
}

void mj::LinearLayout::MoveResizeControl(int16_t x, int16_t y)
{
  // Resizing
  if (this->resizeControlIndex != 0)
  {
    Control* pFirst         = this->controls[this->resizeControlIndex - 1];
    Control* pResizeControl = this->controls[this->resizeControlIndex];
    Control* pSecond        = this->controls[this->resizeControlIndex + 1];

    int16_t dx = x - this->dragStartX;
    int16_t dy = y - this->dragStartY;

    this->MoveResizeControl(pFirst, pResizeControl, pSecond, &dx, &dy);

    this->dragStartX += dx;
    this->dragStartY += dy;

    // Resizing a control will almost always result in invalidation,
    // so we save some complexity by just doing it here.
    ::InvalidateRect(svc::MainWindowHandle(), nullptr, false);
  }
  else
  {
    for (Control* pControl : this->controls)
    {
      pControl->MoveResizeControl(x, y);
    }
  }
}

void mj::LinearLayout::Destroy()
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
          this->dragStartX         = x;
          this->dragStartY         = y;
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
    static_cast<void>(pControl->TranslateClientPoint(&clientX, &clientY));
    if (pControl->OnLeftButtonUp(clientX, clientY))
    {
      return true;
    }
  }

  return false;
}

void mj::LinearLayout::SaveToStringInternal(StringBuilder sb, uint16_t offset)
{
  uint16_t indent = 4;
  sb.Indent(offset).Append(L"controls = {\n");

  for (size_t i = 0; i < this->controls.Size(); i += 2)
  {
    this->controls[i]->SaveToString(sb, offset + indent);
  }

  sb.Indent(offset).Append(L"}\n");
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
