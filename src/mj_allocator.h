#pragma once
#include <stdint.h>
#include <new>

namespace mj
{
  struct Allocation
  {
    void* pAddress  = nullptr;
    size_t numBytes = 0;

    bool Ok()
    {
      return pAddress != nullptr;
    }
  };

  class AllocatorBase
  {
  public:
    virtual void* Allocate(size_t size) = 0;
    virtual void Free(void* ptr)        = 0;

    /// <summary>
    /// Default construction using placement new.
    /// You MUST use this when creating polymorphic types.
    /// </summary>
    template <typename T>
    T* New(void)
    {
      return new (this->Allocate(sizeof(T))) T;
    }

    /// <summary>
    /// Default construction of multiple instances using placement new.
    /// You MUST use this when creating polymorphic types.
    /// </summary>
    template <typename T>
    T* New(size_t numInstances)
    {
      T* pAllocation = static_cast<T*>(this->Allocate(numInstances * sizeof(T)));

      for (size_t i = 0; i < numInstances; i++)
      {
        new (&pAllocation[i]) T;
      }

      return pAllocation;
    }

    mj::Allocation Allocation(size_t size)
    {
      return mj::Allocation{ this->Allocate(size), size };
    }
  };

  class NullAllocator : public AllocatorBase
  {
  public:
    virtual void* Allocate(size_t size) override
    {
      static_cast<void>(size);
      return nullptr;
    }
    virtual void Free(void* ptr) override
    {
      static_cast<void>(ptr);
    }
  };
} // namespace mj
