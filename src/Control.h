#pragma once
#include "mj_allocator.h"

namespace mj
{
  class Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator) = 0;
    virtual void OnPaint()                       = 0;
    virtual void Destroy()                       = 0;

    virtual void OnMouseMove(int16_t x, int16_t y){};
    /// <param name="mkMask">Key State Masks for Mouse Messages (MK_*, see WinUser.h)</param>
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask){};
    /// <param name="mkMask">Key State Masks for Mouse Messages (MK_*, see WinUser.h)</param>
    /// <param name="zDelta">Some ratio of WHEEL_DELTA</param>
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta){};

    int16_t width  = 0;
    int16_t height = 0;
  };
} // namespace mj
