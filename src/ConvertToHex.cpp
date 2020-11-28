#include "ConvertToHex.h"
#include "Direct2D.h"
#include "mj_win32.h"
#include "ErrorExit.h"
#include "minicrt.h"

static constexpr wchar_t s_Mapping[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                                         L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

wchar_t* mj::AllocConvertToHex(char* pBinary, size_t length)
{
  MJ_EXIT_NULL(pBinary);

  // We need three characters for every byte:
  // 2 characters to display the byte (1 byte = 2 printable nibbles)
  // 1 character after every byte for spaces, newlines, or null terminators
  const size_t numChars = length * 3;

  wchar_t* pText = static_cast<wchar_t*>(mj::Win32Alloc(numChars * sizeof(wchar_t)));

  if (pText)
  {
    wchar_t* pCursor = pText;

    // Note MJ: Make this faster. SIMD?
    for (size_t i = 0; i < length; i++)
    {
      // Split nibbles
      const char c  = pBinary[i];
      char hiNibble = (c & 0xF0) >> 4; // Note: Don't need to mask, (c >> 4) is enough
      char loNibble = (c & 0x0F) >> 0; // Note: Mask is enough

      *pCursor++ = s_Mapping[hiNibble];
      *pCursor++ = s_Mapping[loNibble];

      if (i == length - 1) // End of binary
      {
        *pCursor++ = L'\0';
      }
      else if ((i % 16) == 15) // End of line - Note MJ: Hard-coded newline at 16 bytes
      {
        *pCursor++ = L'\n';
      }
      else // Spaces inbetween bytes
      {
        *pCursor++ = L' ';
      }
    }
  }

  return pText;
}

void mj::FreeConvertToHex(wchar_t* pText)
{
  MJ_EXIT_NULL(pText);
  mj::Win32Free(pText);
}
