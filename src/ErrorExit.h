#pragma once
#include "mj_win32.h"
#include "StringBuilder.h"

// Note MJ: We might want to put these in a more general header file.
#define WSTR(x)      L##x
#define XWSTR(x)     WSTR(x)
#define WFILE        XWSTR(__FILE__)
#define __FILENAME__ WFILE

// Macros for use with Win32 functions that set GetLastError
#define MJ_ERR_ZERO(expr)                                                                            \
  do                                                                                                 \
  {                                                                                                  \
    if ((expr) == 0)                                                                                 \
    {                                                                                                \
      mj::ErrorExit(::GetLastError(), mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                                                \
  } while (0)

#define MJ_ERR_IF(expr, x)                                                                           \
  do                                                                                                 \
  {                                                                                                  \
    if ((expr) == (x))                                                                               \
    {                                                                                                \
      mj::ErrorExit(::GetLastError(), mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                                                \
  } while (0)

#define MJ_ERR_NONNULL(expr)                                                                         \
  do                                                                                                 \
  {                                                                                                  \
    if (expr)                                                                                        \
    {                                                                                                \
      mj::ErrorExit(::GetLastError(), mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                                                \
  } while (0)

#define MJ_ERR_ZERO_VALID(expr)                                                           \
  do                                                                                      \
  {                                                                                       \
    ::SetLastError(S_OK);                                                                 \
    if (!(expr))                                                                          \
    {                                                                                     \
      const DWORD _hr = ::GetLastError();                                                 \
      if (_hr)                                                                            \
      {                                                                                   \
        mj::ErrorExit(_hr, mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
      }                                                                                   \
    }                                                                                     \
  } while (0)

#define MJ_ERR_NONZERO(expr)                                                                         \
  do                                                                                                 \
  {                                                                                                  \
    if (expr)                                                                                        \
    {                                                                                                \
      mj::ErrorExit(::GetLastError(), mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                                                \
  } while (0)

#define MJ_ERR_HRESULT(expr)                                                            \
  do                                                                                    \
  {                                                                                     \
    const HRESULT _hr = (expr);                                                         \
    if (_hr != S_OK)                                                                    \
    {                                                                                   \
      mj::ErrorExit(_hr, mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                                   \
  } while (0)

#define MJ_EXIT_NULL(expr)                                                        \
  do                                                                              \
  {                                                                               \
    if (!expr)                                                                    \
    {                                                                             \
      mj::NullExit(mj::String(__FILENAME__), __LINE__, mj::String(XWSTR(#expr))); \
    }                                                                             \
  } while (0)

namespace mj
{
  void ErrorExit(DWORD dw, const String& fileName, int lineNumber, const String& expression);
  void NullExit(const String& fileName, int lineNumber, const String& expression);
} // namespace mj
