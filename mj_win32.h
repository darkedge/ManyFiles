#pragma once

namespace mj
{
  namespace win32
  {
    int Narrow(char* dst, const wchar_t* src, int srcLength, int bufferSize);
    int Widen(wchar_t* dst, const char* src, int srcLength, int bufferSize);
  } // namespace win32
} // namespace mj
