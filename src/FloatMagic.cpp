#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "minicrt.h"

#define XWSTR(x)     WSTR(x)
#define WSTR(x)      L##x
#define WFILE        XWSTR(__FILE__)
#define __FILENAME__ (::mini_wcsrchr("\\" WFILE, '\\') + 1)

#define MJ_ERR_ZERO(expr)                                \
  do                                                     \
  {                                                      \
    if (!(expr))                                         \
    {                                                    \
      ::ErrorExit(__FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                    \
  } while (0)

#define MJ_ERR_NONZERO(expr)                             \
  do                                                     \
  {                                                      \
    if (expr)                                            \
    {                                                    \
      ::ErrorExit(__FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                    \
  } while (0)

/// <summary>
/// Do not recurse into this as it is an exit function.
/// </summary>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void ErrorExit(const wchar_t* fileName, int lineNumber, const wchar_t* expression)
{
  // Retrieve the system error message for the last-error code
  const DWORD dw = ::GetLastError();

  // We specify FORMAT_MESSAGE_ALLOCATE_BUFFER so we need to LocalFree the returned string.
  LPWSTR lpMsgBuf = {};
  static_cast<void>(::FormatMessageW(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr));

  if (lpMsgBuf)
  {
    // Display the error message and exit the process

    // Calculate display string size
    size_t displayStringLength = ::mini_wcslen((LPCWSTR)fileName);
    displayStringLength += ::mini_wcslen((LPCWSTR)expression);
    displayStringLength += ::mini_wcslen((LPCWSTR)lpMsgBuf);
    displayStringLength += 50; // Format string length and decimals

    LPTSTR lpDisplayBuf = static_cast<LPTSTR>(::LocalAlloc(LMEM_ZEROINIT, displayStringLength * sizeof(wchar_t)));
    if (lpDisplayBuf)
    {
      static_cast<void>(::mini_swprintf_s((LPTSTR)lpDisplayBuf, ::LocalSize(lpDisplayBuf) / sizeof(wchar_t),
                                          L"%s:%d - %s failed with error %d: %s", //
                                          fileName,                               //
                                          lineNumber,                             //
                                          expression,                             //
                                          dw,                                     //
                                          lpMsgBuf));

      static_cast<void>(::MessageBoxW(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK));
      static_cast<void>(::LocalFree(lpDisplayBuf));
    }

    static_cast<void>(::LocalFree(lpMsgBuf));
  }

  ::ExitProcess(dw);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;

  case WM_PAINT:
  {
    MJ_UNINITIALIZED PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(hwnd, &ps);
    if (hdc)
    {
      // Don't care if it fails (returns zero, no GetLastError)
      static_cast<void>(::FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1)));

      // Always returns nonzero.
      static_cast<void>(::EndPaint(hwnd, &ps));
    }
    return 0;
  }
  default:
    break;
  }

  return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void FloatMagicMain()
{
  MJ_ERR_ZERO(::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

  // Register the window class.
  static constexpr const auto className = L"Hoi";

  WNDCLASS wc      = {};
  wc.lpfnWndProc   = ::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = className;

  MJ_ERR_ZERO(::RegisterClassW(&wc));

  // Create the window.

  HWND hwnd = ::CreateWindowExW(0,                                                          // Optional window styles.
                                className,                                                  // Window class
                                L"Window Title",                                            // Window text
                                WS_OVERLAPPEDWINDOW,                                        // Window style
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Size and position
                                nullptr,                                                    // Parent window
                                nullptr,                                                    // Menu
                                HINST_THISCOMPONENT,                                        // Instance handle
                                nullptr // Additional application data
  );

  // If the window was previously visible, the return value is nonzero.
  // If the window was previously hidden, the return value is zero.
  static_cast<void>(::ShowWindow(hwnd, SW_SHOW));

  // Run the message loop.

  MSG msg = {};
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    static_cast<void>(::TranslateMessage(&msg));
    static_cast<void>(::DispatchMessageW(&msg));
  }
}
