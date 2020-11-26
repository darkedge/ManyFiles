#pragma once

namespace mj
{
  wchar_t* AllocConvertToHex(char* pBinary, size_t length);
  void FreeConvertToHex(wchar_t* pText);
}