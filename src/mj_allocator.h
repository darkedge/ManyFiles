#pragma once
#include <stdint.h>

namespace mj
{
  class Allocator
  {
  public:
    typedef void* (*AllocFn)(void*, size_t);
    typedef void (*FreeFn)(void*, void*);

    Allocator(void* pContext, AllocFn pAlloc, FreeFn pFree) : pContext(pContext), pAlloc(pAlloc), pFree(pFree)
    {
    }

    void* Allocate(uint32_t size)
    {
      return this->pAlloc(this->pContext, size);
    }

    template <typename T>
    void Free(T* pT)
    {
      this->pFree(this->pContext, pT);
    }

  private:
    void* pContext;
    AllocFn pAlloc;
    FreeFn pFree;
  };
} // namespace mj
