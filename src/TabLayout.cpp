#include "TabLayout.h"
#include "ResourcesD2D1.h"
#include "ServiceLocator.h"

static constexpr const int16_t s_ResizeControlWidth = 8;

void mj::TabLayout::Init(AllocatorBase* pAllocator)
{
  this->pAllocator = pAllocator;
  this->controls.Init(pAllocator);
}

void mj::TabLayout::OnMouseMove(MouseMoveEvent* pMouseMoveEvent)
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

void mj::TabLayout::Destroy()
{
  this->controls.Destroy();

  // We don't own the allocator, so just remove the reference.
  this->pAllocator = nullptr;
}

bool mj::TabLayout::OnLeftButtonDown(int16_t x, int16_t y)
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

bool mj::TabLayout::OnLeftButtonUp(int16_t x, int16_t y)
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

void mj::TabLayout::SaveToStringInternal(StringBuilder sb, uint16_t offset)
{
  uint16_t indent = 4;
  sb.Indent(offset).Append(L"controls = {\n");

  for (size_t i = 0; i < this->controls.Size(); i += 2)
  {
    this->controls[i]->SaveToString(sb, offset + indent);
  }

  sb.Indent(offset).Append(L"}\n");
}
