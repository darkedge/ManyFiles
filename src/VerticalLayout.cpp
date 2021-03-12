#include "VerticalLayout.h"
#include "ResourcesD2D1.h"
#include <d2d1.h>

void mj::VerticalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
{
  static_cast<void>(pRenderTarget);

  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];
    pControl->OnPaint(pRenderTarget);
  }
}

void mj::VerticalLayout::MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t* pDx,
                                           int16_t* pDy)
{
  static_cast<void>(pDx);
  *pDy = mj::clamp<int16_t>(*pDy, pFirst->yParent - pResizeControl->yParent + MIN_PANEL_SIZE,
                            pSecond->height - MIN_PANEL_SIZE);

  pFirst->height += *pDy;
  pResizeControl->yParent += *pDy;
  pSecond->yParent += *pDy;
  pSecond->height -= *pDy;

  pFirst->OnSize();
  pSecond->OnSize();
}

void mj::VerticalLayout::OnSize()
{
  const int16_t numControls = static_cast<int16_t>(this->controls.Size());

  if (numControls > 0)
  {
    int16_t numResizeControls         = numControls / 2;
    int16_t numPanels                 = numResizeControls + 1;
    int16_t totalResizeControlsHeight = numResizeControls * VerticalResizeControl::HEIGHT;

    // TODO: This integer division can leave blank columns if there is a remainder
    int16_t panelHeight = (this->height - totalResizeControlsHeight) / numPanels;
    int16_t y           = 0;

    for (size_t i = 0; i < numControls; i++)
    {
      Control* pControl = this->controls[i];

      pControl->xParent = 0;
      pControl->yParent = y;
      if (!(i & 1))
      {
        pControl->height = panelHeight;
      }
      pControl->width = this->width;
      pControl->OnSize();

      y += pControl->height;
    }
  }
}

void mj::VerticalLayout::Add(mj::Control* pControl)
{
  if (this->controls.Size() > 0)
  {
    Control* pResizeControl = this->pAllocator->New<VerticalResizeControl>();
    pResizeControl->Init(this->pAllocator);
    this->controls.Add(pResizeControl);
  }
  this->controls.Add(pControl);
}

void mj::VerticalResizeControl::Init(AllocatorBase* pAllocator)
{
  static_cast<void>(pAllocator);
  this->height = HEIGHT;
}

void mj::VerticalResizeControl::Paint(ID2D1RenderTarget* pRenderTarget)
{
  auto pBrush = res::d2d1::ResizeControlBrush();
  if (pBrush)
  {
    pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, this->width, this->height), pBrush);
  }
}

bool mj::VerticalResizeControl::OnLeftButtonDown(int16_t x, int16_t y)
{
  static_cast<void>(x);
  static_cast<void>(y);
  return true;
}
