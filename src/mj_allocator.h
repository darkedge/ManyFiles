#pragma once
#include <stdint.h>
#include <new>

namespace mj
{
  struct Allocation
  {
    void* pAddress  = nullptr;
    size_t numBytes = 0;

    bool Ok();
  };

  class AllocatorBase
  {
  public:
    [[nodiscard]] void* Allocate(size_t size);
    void Free(void* ptr);

#if 1 // Interface
  protected:
    [[nodiscard]] virtual void* AllocateInternal(size_t size) = 0;
    virtual void FreeInternal(void* ptr)                      = 0;
    /// <summary>
    /// Return an ASCII string literal for Tracy.
    /// </summary>
    virtual const char* GetName()                             = 0;
#endif

  public:
    /// <summary>
    /// Default construction using placement new.
    /// You MUST use this when creating polymorphic types.
    /// </summary>
    template <typename T>
    [[nodiscard]] T* New(void)
    {
      return new (this->Allocate(sizeof(T))) T;
    }

    /// <summary>
    /// Default construction of multiple instances using placement new.
    /// You MUST use this when creating polymorphic types.
    /// </summary>
    template <typename T>
    [[nodiscard]] T* New(size_t numInstances)
    {
      T* pAllocation = static_cast<T*>(this->Allocate(numInstances * sizeof(T)));

      for (size_t i = 0; i < numInstances; i++)
      {
        new (&pAllocation[i]) T;
      }

      return pAllocation;
    }

    [[nodiscard]] mj::Allocation Allocation(size_t size);
  };

  class NullAllocator : public AllocatorBase
  {
  public:
    [[nodiscard]] virtual void* AllocateInternal(size_t size) override;

    virtual void FreeInternal(void* ptr) override;

    virtual const char* GetName() override;
  };
} // namespace mj
