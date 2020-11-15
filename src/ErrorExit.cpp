#include "ErrorExit.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/// <summary>
/// We do not recurse into this as it is an exit function.
/// </summary>
/// <param name="fileName"></param>
/// <param name="lineNumber"></param>
/// <param name="expression"></param>
void mj::ErrorExit(DWORD dw, const wchar_t* fileName, int lineNumber, const wchar_t* expression)
{
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
