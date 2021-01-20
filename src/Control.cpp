#include "Control.h"
#include "ErrorExit.h"

bool mj::Control::TranslateClientPoint(POINTS* pPointS)
{
  MJ_EXIT_NULL(pPointS);

  MJ_UNINITIALIZED POINT point;
  POINTSTOPOINT(point, *pPointS);

  MJ_UNINITIALIZED RECT rect;
  rect.left   = this->x;
  rect.right  = this->x + this->width;
  rect.top    = this->y;
  rect.bottom = this->y + this->height;

  if (::PtInRect(&rect, point))
  {
    pPointS->x -= this->x;
    pPointS->y -= this->y;
    return true;
  }
}
