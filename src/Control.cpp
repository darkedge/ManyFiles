#include "Control.h"
#include "ErrorExit.h"
#include "../3rdparty/tracy/Tracy.hpp"
#include "ServiceLocator.h"
#include <d2d1.h>

bool mj::Control::TranslateClientPoint(int16_t* pX, int16_t* pY)
{
  MJ_UNINITIALIZED POINT point;
  point.x = *pX;
  point.y = *pY;

  MJ_UNINITIALIZED RECT rect;
  rect.left   = this->xParent;
  rect.right  = this->xParent + this->width;
  rect.top    = this->yParent;
  rect.bottom = this->yParent + this->height;

  if (::PtInRect(&rect, point))
  {
    *pX -= this->xParent;
    *pY -= this->yParent;
    return true;
  }

  return false;
}

void mj::Control::OnPaint()
{
  ZoneScoped;
  auto* pRenderTarget = svc::D2D1RenderTarget();

  // Implies that our brushes are also created
  if (pRenderTarget)
  {
    MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
    pRenderTarget->GetTransform(&transform);
    pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(this->xParent, this->yParent)) * transform);
    MJ_DEFER(pRenderTarget->SetTransform(&transform));

    pRenderTarget->PushAxisAlignedClip(D2D1::RectF(0, 0, this->width, this->height), D2D1_ANTIALIAS_MODE_ALIASED);
    MJ_DEFER(pRenderTarget->PopAxisAlignedClip());

    D2D1_ANTIALIAS_MODE antialiasMode = pRenderTarget->GetAntialiasMode();
    pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
    MJ_DEFER(pRenderTarget->SetAntialiasMode(antialiasMode));

    this->Paint(pRenderTarget);
  }
}
