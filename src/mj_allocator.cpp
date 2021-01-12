#include "mj_allocator.h"
#include "../3rdparty/tracy/Tracy.hpp"
#include "ErrorExit.h"

bool mj::Allocation::Ok()
{
  return pAddress != nullptr;
}

[[nodiscard]] void* mj::AllocatorBase::Allocate(size_t size)
{
  void* ptr = this->AllocateInternal(size);
#ifdef TRACY_ENABLE
  if (ptr)
  {
    TracyAllocN(ptr, size, this->GetName());
  }
#endif
  return ptr;
}
void mj::AllocatorBase::Free(void* ptr)
{
#ifdef TRACY_ENABLE
  TracyFreeN(ptr, this->GetName());
#endif
  this->FreeInternal(ptr);
}

[[nodiscard]] mj::Allocation mj::AllocatorBase::Allocation(size_t size)
{
  return mj::Allocation{ this->Allocate(size), size };
}

[[nodiscard]] void* mj::NullAllocator::AllocateInternal(size_t size)
{
  static_cast<void>(size);
  return nullptr;
}

void mj::NullAllocator::FreeInternal(void* ptr)
{
  static_cast<void>(ptr);
}

const char* mj::NullAllocator::GetName()
{
  return STR(NullAllocator);
}
