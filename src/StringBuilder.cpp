#include "StringBuilder.h"
#include "ErrorExit.h"
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

// The StringBuilder only adds a null terminator in the ToString() function.

static const wchar_t s_IntToWideChar[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                                           L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

mj::String::String(const wchar_t* pString, size_t numChars) : ptr(pString), len(numChars)
{
}

mj::String::String(const wchar_t* pString)
{
  MJ_UNINITIALIZED size_t length;
  if (SUCCEEDED(StringCchLengthW(pString, STRSAFE_MAX_CCH, &length)))
  {
    ptr = pString;
    len = length;
  }
}

void mj::StringBuilder::SetArrayList(ArrayList<wchar_t>* pArrayList)
{
  this->pArrayList = pArrayList;
}

mj::StringBuilder& mj::StringBuilder::Append(int32_t integer)
{
  // TODO: Do we actually ever print a null terminator?
  wchar_t buf[12]; // E.g. ['-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0']
  wchar_t* pEnd   = buf + MJ_COUNTOF(buf);
  wchar_t* pHead  = pEnd;
  bool isNegative = integer < 0;

  if (isNegative)
  {
    integer = -integer;
  }

  do
  {
    int32_t div = integer / 10;                      // Calculate next value
    --pHead;                                         // fill it backwards
    *pHead  = s_IntToWideChar[integer - (div * 10)]; // Calculate rest value
    integer = div;                                   // Decimal 10 conversion
  } while (integer);

  // Add sign
  if (isNegative)
  {
    --pHead;
    *pHead = '-';
  }

  ptrdiff_t numChars = pEnd - pHead;
  String string(pHead, numChars);
  return this->Append(string);
}

mj::StringBuilder& mj::StringBuilder::AppendHex32(uint32_t dw)
{
  wchar_t buf[8]; // E.g. ['-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0']
  wchar_t* pEnd  = buf + MJ_COUNTOF(buf);
  wchar_t* pHead = pEnd;

  for (int i = 0; i < 8; i++)
  {
    --pHead;
    *pHead = s_IntToWideChar[dw & 0x0F];
    dw >>= 4;
  }

  ptrdiff_t numChars = pEnd - pHead;
  String string(pHead, numChars);
  return this->Append(string);
}

mj::StringBuilder& mj::StringBuilder::Append(const String& string)
{
  wchar_t* pDest = this->pArrayList->Reserve(string.len);

  if (pDest)
  {
    // Use memcpy instead of strcpy to prevent copying the null terminator
    static_cast<void>(memcpy(pDest, string.ptr, string.len * sizeof(wchar_t)));
  }

  return *this;
}

mj::StringBuilder& mj::StringBuilder::Append(const wchar_t* pStringLiteral)
{
  MJ_UNINITIALIZED size_t numChars;
  if (SUCCEEDED(StringCchLengthW(pStringLiteral, STRSAFE_MAX_CCH, &numChars)))
  {
    return this->Append(String(pStringLiteral, numChars));
  }

  return *this;
}

mj::String mj::StringBuilder::ToString()
{
  // Make sure the string is null-terminated
  wchar_t* pLast = this->pArrayList->Reserve(1);
  if (pLast)
  {
    *pLast = L'\0';
  }
  else
  {
    auto size = this->pArrayList->Size();
    if (size > 0)
    {
      (*this->pArrayList)[size - 1] = L'\0';
    }
  }

  MJ_UNINITIALIZED size_t length;
  MJ_ERR_HRESULT(StringCchLengthW(this->pArrayList->Get(), STRSAFE_MAX_CCH, &length));

  return String(this->pArrayList->begin(), length);
}
