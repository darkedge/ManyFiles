#include "ncrt_memory.h"
#include "ErrorExit.h"

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

// musl libc
#define WT size_t
#define WS (sizeof(WT))
#pragma function(memmove)
  void* memmove(void* dest, const void* src, size_t n)
  {
    char* d       = (char*)dest;
    const char* s = (const char*)src;

    if (d == s)
      return d;
    if (s + n <= d || d + n <= s)
      return memcpy(d, s, n);

    if (d < s)
    {
      if ((uintptr_t)s % WS == (uintptr_t)d % WS)
      {
        while ((uintptr_t)d % WS)
        {
          if (!n--)
            return dest;
          *d++ = *s++;
        }
        for (; n >= WS; n -= WS, d += WS, s += WS)
          *(WT*)d = *(WT*)s;
      }
      for (; n; n--)
        *d++ = *s++;
    }
    else
    {
      if ((uintptr_t)s % WS == (uintptr_t)d % WS)
      {
        while ((uintptr_t)(d + n) % WS)
        {
          if (!n--)
            return dest;
          d[n] = s[n];
        }
        while (n >= WS)
          n -= WS, *(WT*)(d + n) = *(WT*)(s + n);
      }
      while (n)
        n--, d[n] = s[n];
    }

    return dest;
  }
}

void* operator new(size_t n)
{
  return HeapAlloc(GetProcessHeap(), 0, n);
}

void* operator new[](size_t n)
{
  return HeapAlloc(GetProcessHeap(), 0, n);
}

void operator delete(void* p, size_t sz)
{
  static_cast<void>(sz);
  static_cast<void>(HeapFree(GetProcessHeap(), 0, p));
}

void operator delete[](void* p)
{
  static_cast<void>(HeapFree(GetProcessHeap(), 0, p));
}

void* mj::malloc(size_t sz)
{
  auto p = HeapAlloc(GetProcessHeap(), 0, sz);
  // HeapAlloc does not call SetLastError.
  MJ_EXIT_NULL(p);
  return p;
}

void* mj::realloc(void* p, size_t newsz)
{
  if (p)
  {
    auto ptr = HeapReAlloc(GetProcessHeap(), 0, p, newsz);
    // HeapReAlloc does not call SetLastError.
    MJ_EXIT_NULL(ptr);
    return ptr;
  }
  else
  {
    return mj::malloc(newsz);
  }
}

void mj::free(void* p)
{
  HeapFree(GetProcessHeap(), 0, p);
}
