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
    UINT32 numLines;
  };

  void HexEditSetBinary(BinaryBlob blob);
  void HexEditOnSize(WORD newClientHeight);
  void HexEditOnScroll(WORD scrollType);

  WideString HexEditGetBuffer();
  BinaryBlob HexEditGetBinary();
} // namespace mj
