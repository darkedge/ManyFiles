#pragma once
#include "mj_allocator.h"

namespace mj
{
  class TracyAllocatorWrapper : public AllocatorBase
  {
  private:
    AllocatorBase* pAllocator = nullptr;
    const char* pName = nullptr;

#if 1 // Interface implementation
  public:
    [[nodiscard]] virtual void* Allocate(size_t size) override;
    virtual void Free(void* ptr) override;
#endif

  public:
    void Init(AllocatorBase* pAllocator, const char* pName);
    void Destroy();
  };
} // namespace mj
