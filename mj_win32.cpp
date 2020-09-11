#include "mj_win32.h"

int mj::win32::Narrow(char* dst, const wchar_t* src, int srcLength, int bufferSize) noexcept
{
  int numBytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, srcLength, nullptr, 0, nullptr, nullptr);
  if (numBytes > 0)
  {
    // Buffer overrun protection
    if (numBytes > bufferSize)
    {
      numBytes            = bufferSize - 1;
      dst[bufferSize - 1] = '\0';
    }

    numBytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, src, srcLength, dst, numBytes, nullptr, nullptr);
  }
  return numBytes;
}

int mj::win32::Widen(wchar_t* dst, const char* src, int srcLength, int bufferSize) noexcept
{
  int numCharacters = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, srcLength, nullptr, 0);
  if (numCharacters > 0)
  {
    // Buffer overrun protection
    if (numCharacters > bufferSize)
    {
      numCharacters       = bufferSize - 1;
      dst[bufferSize - 1] = '\0';
    }

    numCharacters = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, src, srcLength, dst, numCharacters);
  }
  return numCharacters;
}
