#include "mj_win32.h"
#include "ErrorExit.h"
#include "minicrt.h"

void* mj::Win32Alloc(size_t size, void* pContext)
{
  static_cast<void>(pContext);
  return ::VirtualAlloc(nullptr,                  //
                        size,                     //
                        MEM_COMMIT | MEM_RESERVE, //
                        PAGE_READWRITE);
}

void mj::Win32Free(void* ptr, void* pContext)
{
  MJ_EXIT_NULL(ptr);
  static_cast<void>(pContext);
  ::VirtualFree(ptr, 0, MEM_RELEASE);
}
