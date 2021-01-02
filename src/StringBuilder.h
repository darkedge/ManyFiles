#pragma once
#include "mj_common.h"

namespace mj
{
  /// <summary>
  /// Wide string with known length
  /// </summary>
  struct String
  {
    const wchar_t* ptr = nullptr;
    size_t len         = 0; // Number of characters, compatible with DirectWrite "string length"
    String(const wchar_t* pString, size_t numChars);
    String(const wchar_t* pString);
  };

  class StringBuilder
  {
  private:
    ArrayList<wchar_t> *pArrayList;

  public:
    void SetArrayList(ArrayList<wchar_t>* pArrayList);

    StringBuilder& Append(const String& string);
    StringBuilder& Append(const wchar_t* pStringLiteral);
    StringBuilder& Append(int32_t integer);
    StringBuilder& AppendHex32(uint32_t dw);
    String ToString();
  };
} // namespace mj
