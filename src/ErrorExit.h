#pragma once
#include "mj_win32.h"
#include "mj_string.h"

// Macros for use with Win32 functions that set GetLastError
#define MJ_ERR_ZERO(expr)                                          \
  do                                                               \
  {                                                                \
    if ((expr) == 0)                                               \
    {                                                              \
      MJ_UNINITIALIZED mj::StringView _fileName;                   \
      MJ_UNINITIALIZED mj::StringView _expr;                       \
      _fileName.Init(__FILENAME__);                                \
      _expr.Init(XWSTR(#expr));                                    \
      mj::ErrorExit(::GetLastError(), _fileName, __LINE__, _expr); \
    }                                                              \
  } while (0)

#define MJ_ERR_IF(expr, x)                                         \
  do                                                               \
  {                                                                \
    if ((expr) == (x))                                             \
    {                                                              \
      MJ_UNINITIALIZED mj::StringView _fileName;                   \
      MJ_UNINITIALIZED mj::StringView _expr;                       \
      _fileName.Init(__FILENAME__);                                \
      _expr.Init(XWSTR(#expr));                                    \
      mj::ErrorExit(::GetLastError(), _fileName, __LINE__, _expr); \
    }                                                              \
  } while (0)

#define MJ_ERR_NONNULL(expr)                                       \
  do                                                               \
  {                                                                \
    if (expr)                                                      \
    {                                                              \
      MJ_UNINITIALIZED mj::StringView _fileName;                   \
      MJ_UNINITIALIZED mj::StringView _expr;                       \
      _fileName.Init(__FILENAME__);                                \
      _expr.Init(XWSTR(#expr));                                    \
      mj::ErrorExit(::GetLastError(), _fileName, __LINE__, _expr); \
    }                                                              \
  } while (0)

#define MJ_ERR_ZERO_VALID(expr)                         \
  do                                                    \
  {                                                     \
    ::SetLastError(S_OK);                               \
    if (!(expr))                                        \
    {                                                   \
      const DWORD _hr = ::GetLastError();               \
      if (_hr)                                          \
      {                                                 \
        MJ_UNINITIALIZED mj::StringView _fileName;      \
        MJ_UNINITIALIZED mj::StringView _expr;          \
        _fileName.Init(__FILENAME__);                   \
        _expr.Init(XWSTR(#expr));                       \
        mj::ErrorExit(_hr, _fileName, __LINE__, _expr); \
      }                                                 \
    }                                                   \
  } while (0)

#define MJ_ERR_NONZERO(expr)                                       \
  do                                                               \
  {                                                                \
    if (expr)                                                      \
    {                                                              \
      MJ_UNINITIALIZED mj::StringView _fileName;                   \
      MJ_UNINITIALIZED mj::StringView _expr;                       \
      _fileName.Init(__FILENAME__);                                \
      _expr.Init(XWSTR(#expr));                                    \
      mj::ErrorExit(::GetLastError(), _fileName, __LINE__, _expr); \
    }                                                              \
  } while (0)

#define MJ_ERR_HRESULT(expr)                          \
  do                                                  \
  {                                                   \
    const HRESULT _hr = (expr);                       \
    if (_hr != S_OK)                                  \
    {                                                 \
      MJ_UNINITIALIZED mj::StringView _fileName;      \
      MJ_UNINITIALIZED mj::StringView _expr;          \
      _fileName.Init(__FILENAME__);                   \
      _expr.Init(XWSTR(#expr));                       \
      mj::ErrorExit(_hr, _fileName, __LINE__, _expr); \
    }                                                 \
  } while (0)

#define MJ_EXIT_NULL(expr)                       \
  do                                             \
  {                                              \
    if (!expr)                                   \
    {                                            \
      MJ_UNINITIALIZED mj::StringView _fileName; \
      MJ_UNINITIALIZED mj::StringView _expr;     \
      _fileName.Init(__FILENAME__);              \
      _expr.Init(XWSTR(#expr));                  \
      mj::NullExit(_fileName, __LINE__, _expr);  \
    }                                            \
  } while (0)

namespace mj
{
  void ErrorExit(DWORD dw, const StringView& fileName, int lineNumber, const StringView& expression);
  void NullExit(const StringView& fileName, int lineNumber, const StringView& expression);
} // namespace mj
