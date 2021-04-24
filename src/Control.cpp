#include "Control.h"
#include "ErrorExit.h"
#include "../3rdparty/tracy/Tracy.hpp"
#include "ServiceLocator.h"
#include "ResourcesD2D1.h"
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

void mj::Control::OnPaint(ID2D1RenderTarget* pRenderTarget)
{
  ZoneScoped;

  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
  pRenderTarget->GetTransform(&transform);
  pRenderTarget->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(this->xParent, this->yParent)) * transform);
  MJ_DEFER(pRenderTarget->SetTransform(&transform));

  D2D1_RECT_F rect = D2D1::RectF(0, 0, this->width, this->height);
  pRenderTarget->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_ALIASED);
  MJ_DEFER(pRenderTarget->PopAxisAlignedClip());

  pRenderTarget->FillRectangle(rect, res::d2d1::Brush());

  D2D1_ANTIALIAS_MODE antialiasMode = pRenderTarget->GetAntialiasMode();
  pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
  MJ_DEFER(pRenderTarget->SetAntialiasMode(antialiasMode));

  this->Paint(pRenderTarget);
}
