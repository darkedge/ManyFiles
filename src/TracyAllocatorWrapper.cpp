#include "TracyAllocatorWrapper.h"
#include "ErrorExit.h"
#include "../3rdparty/tracy/Tracy.hpp"

void* mj::TracyAllocatorWrapper::Allocate(size_t size)
{
  void* ptr = this->pAllocator->Allocate(size);
  if (ptr)
  {
    TracyAllocN(ptr, size, pName);
  }
  return ptr;
}

void mj::TracyAllocatorWrapper::Free(void* ptr)
{
  TracyFreeN(ptr, pName);
  this->pAllocator->Free(ptr);
}

void mj::TracyAllocatorWrapper::Init(AllocatorBase* pAllocator, const char* pName)
{
  this->pAllocator = pAllocator;
  this->pName = pName;
}

void mj::TracyAllocatorWrapper::Destroy()
{
  this->pAllocator = nullptr;
}
