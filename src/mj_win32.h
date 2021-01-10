#pragma once
#include <Windows.h>
#include "mj_allocator.h"

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

    [[nodiscard]]
    virtual void* Allocate(size_t size) override
    {
      return ::VirtualAlloc(nullptr,                  //
                            size,                     //
                            MEM_COMMIT | MEM_RESERVE, //
                            PAGE_READWRITE);
    }

    virtual void Free(void* ptr) override
    {
      ::VirtualFree(ptr, 0, MEM_RELEASE);
    }
  };

  /// <summary>
  /// Uses HeapAlloc/HeapFree.
  /// Does not initialize memory ot zero.
  /// Is NOT thread-safe.
  /// </summary>
  class HeapAllocator : public AllocatorBase
  {
  private:
    HANDLE pHeap;

  public:
    void Init()
    {
      pHeap = ::GetProcessHeap();
    }

    [[nodiscard]]
    virtual void* Allocate(size_t size) override
    {
      return ::HeapAlloc(pHeap, 0, size);
    }

    virtual void Free(void* ptr) override
    {
      ::HeapFree(pHeap, 0, ptr);
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
