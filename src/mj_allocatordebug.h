#pragma once
#include "mj_allocator.h"
#include "mj_hashtable.h"
#include "ErrorExit.h"

namespace mj
{
  class AllocatorDebug : public AllocatorBase
  {
  private:
    AllocatorBase* pAllocator = nullptr;
    HashTable allocations;

  public:
    void Init(AllocatorBase* pAllocator)
    {
      this->pAllocator = pAllocator;
      this->allocations.Init(pAllocator);
    }

    void Destroy()
    {
      for (const auto& it : this->allocations)
      {
        MJ_EXIT_NULL(it);
      }
      this->allocations.Destroy();
      this->pAllocator = nullptr;
    }

    virtual void* Allocate(size_t size) override
    {
      void* ptr = this->pAllocator->Allocate(size);
      if (ptr)
      {
        this->allocations.Insert(ptr);
      }
      return ptr;
    }

    virtual void Free(void* ptr) override
    {
      this->allocations.Remove(ptr);
      this->pAllocator->Free(ptr);
    }
  };
} // namespace mj
