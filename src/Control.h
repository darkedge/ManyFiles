#pragma once
#include "mj_allocator.h"

namespace mj
{
  class Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator)                      = 0;
    virtual void OnPaint()                                            = 0;
    virtual void Destroy()                                            = 0;
    virtual void OnMouseMove(int16_t x, int16_t y)                    = 0;
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask) = 0;

    int16_t width = 0;
    int16_t height = 0;
  };
} // namespace mj
