#pragma once
#include "mj_allocator.h"

namespace mj
{
  class IControl
  {
  public:
    virtual void Init(AllocatorBase* pAllocator) = 0;
    virtual void Paint()                         = 0;
    virtual void Destroy()                       = 0;
  };
} // namespace mj
