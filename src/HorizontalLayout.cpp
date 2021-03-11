#include "HorizontalLayout.h"
#include "ResourcesD2D1.h"
#include <d2d1.h>

void mj::HorizontalLayout::Paint(ID2D1RenderTarget* pRenderTarget)
{
  static_cast<void>(pRenderTarget);

  for (size_t i = 0; i < this->controls.Size(); i++)
  {
    Control* pControl = this->controls[i];
    pControl->OnPaint(pRenderTarget);
  }
}

void mj::HorizontalLayout::MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t* pDx,
                                             int16_t* pDy)
{
  static_cast<void>(pDy);
  *pDx = mj::clamp<int16_t>(*pDx, pFirst->xParent - pResizeControl->xParent + MIN_PANEL_SIZE,
                            pSecond->width - MIN_PANEL_SIZE);

  pFirst->width += *pDx;
  pResizeControl->xParent += *pDx;
  pSecond->xParent += *pDx;
  pSecond->width -= *pDx;
}

void mj::HorizontalLayout::OnSize()
{
  const int16_t numControls = static_cast<int16_t>(this->controls.Size());

  if (numControls > 0)
  {
    int16_t numResizeControls        = numControls / 2;
    int16_t numPanels                = numResizeControls + 1;
    int16_t totalResizeControlsWidth = numResizeControls * HorizontalResizeControl::WIDTH;

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
    Control* pResizeControl = this->pAllocator->New<HorizontalResizeControl>();
    pResizeControl->Init(this->pAllocator);
    this->controls.Add(pResizeControl);
  }
  this->controls.Add(pControl);
}

void mj::HorizontalResizeControl::Init(AllocatorBase* pAllocator)
{
  static_cast<void>(pAllocator);
  this->width = WIDTH;
}

void mj::HorizontalResizeControl::Paint(ID2D1RenderTarget* pRenderTarget)
{
  auto pBrush = res::d2d1::ResizeControlBrush();
  if (pBrush)
  {
    pRenderTarget->FillRectangle(D2D1::RectF(0.0f, 0.0f, this->width, this->height), pBrush);
  }
}

bool mj::HorizontalResizeControl::OnLeftButtonDown(int16_t x, int16_t y)
{
  static_cast<void>(x);
  static_cast<void>(y);
  return true;
}
