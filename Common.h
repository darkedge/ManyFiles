#pragma once

// The following macros define the minimum required platform.  The minimum required platform
// is the earliest version of Windows, Internet Explorer etc. that has the necessary features to run
// your application.  The macros work by enabling all features available on platform versions up to and
// including the version specified.

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.

#ifndef WINVER // Minimum platform is Windows 7
#define WINVER 0x0601
#endif

#ifndef _WIN32_WINNT // Minimum platform is Windows 7
#define _WIN32_WINNT 0x0601
#endif

#ifndef _WIN32_WINDOWS // Minimum platform is Windows 7
#define _WIN32_WINDOWS 0x0601
#endif

#ifndef WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX // Use the standard's templated min/max
#define NOMINMAX
#endif

#ifndef _USE_MATH_DEFINES // We do want PI defined
#define _USE_MATH_DEFINES
#endif

#include "mj_common.h"
#include "mj_math.h"
#define mj_min(a, b) (a < b ? a : b)
#define mj_max(a, b) (a > b ? a : b)

// Windows headers:

#include <windows.h>
#include <windowsx.h>
#include <winnls.h>
#include <unknwn.h>

#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <strsafe.h>
#include <commdlg.h>

// Common macro definitions:

#if !defined(NDEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

// Use the double macro technique to stringize the actual value of s
#define STRINGIZE_(s)  STRINGIZE2_(s)
#define STRINGIZE2_(s) #s

#define FAILURE_LOCATION "\r\nFunction: " __FUNCTION__ "\r\nLine: " STRINGIZE_(__LINE__)

// Ignore unreferenced parameters, since they are very common
// when implementing callbacks.
#pragma warning(disable : 4100)

// Application specific headers/functions:

#define APPLICATION_TITLE "FloatMagic"

// Needed text editor backspace deletion.
inline bool IsSurrogate(UINT32 ch) noexcept
{
  // 0xD800 <= ch <= 0xDFFF
  return (ch & 0xF800) == 0xD800;
}

inline bool IsHighSurrogate(UINT32 ch) noexcept
{
  // 0xD800 <= ch <= 0xDBFF
  return (ch & 0xFC00) == 0xD800;
}

inline bool IsLowSurrogate(UINT32 ch) noexcept
{
  // 0xDC00 <= ch <= 0xDFFF
  return (ch & 0xFC00) == 0xDC00;
}
