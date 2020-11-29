#include "HexEdit.h"
#include "Direct2D.h"
#include "mj_win32.h"
#include "ErrorExit.h"
#include <strsafe.h>
#include "mj_common.h"
#include "FloatMagic.h"
#include "minicrt.h"

static constexpr wchar_t s_Mapping[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                                         L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

static mj::BinaryBlob s_pBinary;
static mj::WideString s_pBuffer;

static void AllocConvertToHex()
{
  // We need three characters for every byte:
  // 2 characters to display the byte (1 byte = 2 printable nibbles)
  // 1 character after every byte for spaces, newlines, or null terminators
  const UINT32 numChars = s_pBinary.length * 3;

  if (s_pBuffer.pString)
  {
    mj::Win32Free(s_pBuffer.pString);
  }

  s_pBuffer.pString  = static_cast<wchar_t*>(mj::Win32Alloc(numChars * sizeof(wchar_t)));
  s_pBuffer.numLines = 1;

  if (s_pBuffer.pString)
  {
    wchar_t* pCursor = s_pBuffer.pString;

    // Note MJ: Make this faster. SIMD?
    for (size_t i = 0; i < s_pBinary.length; i++)
    {
      // Split nibbles
      const char c  = s_pBinary.pBinary[i];
      char hiNibble = (c & 0xF0) >> 4; // Note: Don't need to mask, (c >> 4) is enough
      char loNibble = (c & 0x0F) >> 0; // Note: Mask is enough

      *pCursor++ = s_Mapping[hiNibble];
      *pCursor++ = s_Mapping[loNibble];

      if (i == s_pBinary.length - 1) // End of binary
      {
        *pCursor++ = L'\0';
      }
      else if ((i % 16) == 15) // End of line - Note MJ: Hard-coded newline at 16 bytes
      {
        *pCursor++ = L'\n';
        s_pBuffer.numLines++;
      }
      else // Spaces inbetween bytes
      {
        *pCursor++ = L' ';
      }
    }
  }

  // String length calculation
  // StringCchLengthW outputs a size_t but we can only handle uint32_t characters.
  MJ_UNINITIALIZED size_t stringLength;
  MJ_ERR_HRESULT(::StringCchLengthW(s_pBuffer.pString, numChars, &stringLength));
  s_pBuffer.length = static_cast<UINT32>(stringLength);
}

void mj::HexEditSetBuffer(mj::WideString pBuffer)
{
  s_pBuffer = pBuffer;
  mj::RenderHexEditBuffer();
}

void mj::HexEditSetBinary(mj::BinaryBlob blob)
{
  if (s_pBinary.pBinary)
  {
    mj::Win32Free(s_pBinary.pBinary);
  }

  s_pBinary = blob;

  ::AllocConvertToHex();
}

mj::WideString mj::HexEditGetBuffer()
{
  return s_pBuffer;
}

mj::BinaryBlob mj::HexEditGetBinary()
{
  return s_pBinary;
}

void mj::HexEditOnSize(WORD newClientHeight)
{
  // We're scrolling on the pixel level, using the pixel height from
  // ITextLayout metrics and the client area size.

  SCROLLINFO si = {};
  si.cbSize     = sizeof(si);
  si.fMask      = SIF_RANGE | SIF_PAGE;
  si.nMin       = 0;
  si.nMax       = newClientHeight;
  si.nPage      = mj::GetRenderedTextHeight();

  HWND hWnd = mj::GetMainWindowHandle();
  if (hWnd)
  {
    // The return value is the current position of the scroll box.
    static_cast<void>(::SetScrollInfo(hWnd, SB_VERT, &si, TRUE));
  }
}

void mj::HexEditOnScroll(WORD scrollType)
{
  HWND hWnd             = mj::GetMainWindowHandle();

  SCROLLINFO scrollInfo = {};
  scrollInfo.cbSize     = sizeof(scrollInfo);
  scrollInfo.fMask      = SIF_ALL;
  MJ_ERR_ZERO(::GetScrollInfo(hWnd, SB_VERT, &scrollInfo));

  // Save the position for comparison later on.
  const int yPos = scrollInfo.nPos;
  switch (scrollType)
  {
  case SB_TOP: // User clicked the HOME keyboard key.
    scrollInfo.nPos = scrollInfo.nMin;
    break;
  case SB_BOTTOM: // User clicked the END keyboard key.
    scrollInfo.nPos = scrollInfo.nMax;
    break;
  case SB_LINEUP: // User clicked the top arrow.
    scrollInfo.nPos -= 10;
    break;
  case SB_LINEDOWN: // User clicked the bottom arrow.
    scrollInfo.nPos += 10;
    break;
  case SB_PAGEUP: // User clicked the scroll bar shaft above the scroll box.
    scrollInfo.nPos -= scrollInfo.nPage;
    break;
  case SB_PAGEDOWN: // User clicked the scroll bar shaft below the scroll box.
    scrollInfo.nPos += scrollInfo.nPage;
    break;
  case SB_THUMBTRACK: // User dragged the scroll box.
    scrollInfo.nPos = scrollInfo.nTrackPos;
    break;

  default:
    break;
  }

  // Set the position and then retrieve it.  Due to adjustments
  // by Windows it may not be the same as the value set.
  scrollInfo.fMask = SIF_POS;
  static_cast<void>(::SetScrollInfo(hWnd, SB_VERT, &scrollInfo, TRUE));
  MJ_ERR_ZERO(::GetScrollInfo(hWnd, SB_VERT, &scrollInfo));

  // If the position has changed, scroll window and update it.
  if (scrollInfo.nPos != yPos)
  {
    MJ_ERR_ZERO(::ScrollWindow(hWnd, 0, yPos - scrollInfo.nPos, nullptr, nullptr));

    // If the function fails, the return value is zero.
    // GetLastError is not specified in the documentation.
    static_cast<void>(::UpdateWindow(hWnd));
  }
}
