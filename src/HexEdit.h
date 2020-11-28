#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace mj
{
  /// <summary>
  /// Pass by value.
  /// </summary>
  struct BinaryBlob
  {
    char* pBinary;
    UINT32 length;
  };

  /// <summary>
  /// Pass by value.
  /// </summary>
  struct WideString
  {
    PWCHAR pString;
    UINT32 length;
  };

  void HexEditSetBuffer(WideString pBuffer);
  void HexEditSetBinary(BinaryBlob blob);

  WideString HexEditGetBuffer();
  BinaryBlob HexEditGetBinary();
} // namespace mj
