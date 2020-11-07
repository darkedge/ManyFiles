#include "mj_win32.h"

void* mj::Win32Alloc(void* pContext, size_t size)
{
  static_cast<void>(pContext);
  return ::VirtualAlloc(nullptr,                  //
                        size,                     //
                        MEM_COMMIT | MEM_RESERVE, //
                        PAGE_READWRITE);
}

void mj::Win32Free(void* pContext, void* ptr)
{
  static_cast<void>(pContext);
  ::VirtualFree(ptr, 0, MEM_RELEASE);
}
