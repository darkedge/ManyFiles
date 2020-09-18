#include "ncrt_memory.h"

#include <Windows.h>
#include <intrin.h>

extern "C"
{
#pragma function(memset)
  void* __cdecl memset(void* dest, int value, size_t num)
  {
    __stosb(static_cast<unsigned char*>(dest), static_cast<unsigned char>(value), num);
    return dest;
  }

#pragma function(memcpy)
  void* __cdecl memcpy(void* dest, const void* src, size_t num)
  {
    __movsb(static_cast<unsigned char*>(dest), static_cast<const unsigned char*>(src), num);
    return dest;
  }

// https://stackoverflow.com/questions/5017659/implementing-memcmp
#pragma function(memcmp)
  int memcmp(const void* buf1, const void* buf2, size_t count)
  {
    if (!count)
      return (0);

    while (--count && *(char*)buf1 == *(char*)buf2)
    {
      buf1 = (char*)buf1 + 1;
      buf2 = (char*)buf2 + 1;
    }

    return (*((unsigned char*)buf1) - *((unsigned char*)buf2));
  }
}

void* operator new(size_t n)
{
  return HeapAlloc(GetProcessHeap(), NULL, n);
}

void* operator new[](size_t n)
{
  return HeapAlloc(GetProcessHeap(), NULL, n);
}

void operator delete(void* p, size_t sz)
{
  (void)sz;
  HeapFree(GetProcessHeap(), NULL, p);
}

void operator delete[](void* p)
{
  HeapFree(GetProcessHeap(), NULL, p);
}
