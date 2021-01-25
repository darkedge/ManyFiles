#include "Control.h"
#include "ErrorExit.h"

bool mj::Control::TranslateClientPoint(int16_t* pX, int16_t* pY)
{
  MJ_UNINITIALIZED POINT point;
  point.x = *pX;
  point.y = *pY;

  MJ_UNINITIALIZED RECT rect;
  rect.left   = this->x;
  rect.right  = this->x + this->width;
  rect.top    = this->y;
  rect.bottom = this->y + this->height;

  if (::PtInRect(&rect, point))
  {
    *pX -= this->x;
    *pY -= this->y;
    return true;
  }

  return false;
}
