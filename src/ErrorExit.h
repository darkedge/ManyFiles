#pragma once
#include "mj_win32.h"
#include "minicrt.h"

// Note MJ: We might want to put these in a more general header file.
#define XWSTR(x)     WSTR(x)
#define WSTR(x)      L##x
#define WFILE        XWSTR(__FILE__)
#define __FILENAME__ (::mini_wcsrchr("\\" WFILE, '\\') + 1)

// Macros for use with Win32 functions that set GetLastError
#define MJ_ERR_ZERO(expr)                                                    \
  do                                                                         \
  {                                                                          \
    if (!(expr))                                                             \
    {                                                                        \
      mj::ErrorExit(::GetLastError(), __FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                                        \
  } while (0)

#define MJ_ERR_ZERO_VALID(expr)                                   \
  do                                                              \
  {                                                               \
    ::SetLastError(S_OK);                                         \
    if (!(expr))                                                  \
    {                                                             \
      const DWORD _hr = ::GetLastError();                         \
      if (_hr)                                                    \
      {                                                           \
        mj::ErrorExit(_hr, __FILENAME__, __LINE__, XWSTR(#expr)); \
      }                                                           \
    }                                                             \
  } while (0)

#define MJ_ERR_NULL(ptr) MJ_ERR_ZERO(ptr)

#define MJ_ERR_NONZERO(expr)                                                 \
  do                                                                         \
  {                                                                          \
    if (expr)                                                                \
    {                                                                        \
      mj::ErrorExit(::GetLastError(), __FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                                        \
  } while (0)

#define MJ_ERR_HRESULT(expr)                                    \
  do                                                            \
  {                                                             \
    const HRESULT _hr = (expr);                                 \
    if (_hr != S_OK)                                            \
    {                                                           \
      mj::ErrorExit(_hr, __FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                           \
  } while (0)

#define MJ_EXIT_NULL(expr)                                \
  do                                                      \
  {                                                       \
    if (!expr)                                            \
    {                                                     \
      mj::NullExit(__FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                     \
  } while (0)

namespace mj
{
  void ErrorExit(DWORD dw, const wchar_t* fileName, int lineNumber, const wchar_t* expression);
  void NullExit(const wchar_t* fileName, int lineNumber, const wchar_t* expression);
} // namespace mj
