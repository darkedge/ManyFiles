#pragma once
#include "mj_allocator.h"
#include "mj_macro.h"

// __ImageBase is better than GetCurrentModule()
// Can be cast to a HINSTANCE
#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define MJ_SAFE_RELEASE(x) \
  do                       \
  {                        \
    if (x)                 \
    {                      \
      x->Release();        \
      x = nullptr;         \
    }                      \
  } while (0)

namespace mj
{
  /// <summary>
  /// Uses VirtualAlloc/VirtualFree.
  /// Use sparingly (i.e. once), for large allocations.
  /// </summary>
  class VirtualAllocator : public AllocatorBase
  {
  private:
    LPVOID pBaseAddress;

  public:
    void Init(LPVOID pBaseAddress)
    {
      this->pBaseAddress = pBaseAddress;
    }

  protected:
    [[nodiscard]] virtual void* AllocateInternal(size_t size) override
    {
      return ::VirtualAlloc(nullptr,                  //
                            size,                     //
                            MEM_COMMIT | MEM_RESERVE, //
                            PAGE_READWRITE);
    }

    virtual void FreeInternal(void* ptr) override
    {
      ::VirtualFree(ptr, 0, MEM_RELEASE);
    }

    virtual const char* GetName() override
    {
      return STR(VirtualAllocator);
    }
  };

  /// <summary>
  /// Uses HeapAlloc/HeapFree.
  /// Does not initialize memory to zero.
  /// Is thread-safe.
  /// </summary>
  class HeapAllocator : public AllocatorBase
  {
  protected:
    [[nodiscard]] virtual void* AllocateInternal(size_t size) override
    {
      return ::HeapAlloc(::GetProcessHeap(), 0, size);
    }

    virtual void FreeInternal(void* ptr) override
    {
      ::HeapFree(::GetProcessHeap(), 0, ptr);
    }

    virtual const char* GetName() override
    {
      return STR(HeapAllocator);
    }
  };

  template <typename T>
  struct DeferRelease
  {
    T* ptr;

    ~DeferRelease()
    {
      ptr->Release();
    }

    T* operator->()
    {
      return this->ptr;
    }

    T** operator&()
    {
      return &this->ptr;
    }

    template <typename U>
    operator U()
    {
      return this->ptr;
    }
  };
} // namespace mj
