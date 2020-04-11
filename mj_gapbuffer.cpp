#include "mj_gapbuffer.h"
#include "mj_win32.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

static void CursorUpdate(mj::GapBuffer* pBuf)
{
  if (pBuf->pCursor < pBuf->pGapBegin)
  {
    // Copy [pCursor...pGapBegin] to pGapEnd
    size_t numBytes = pBuf->pGapBegin - pBuf->pCursor;
    pBuf->pGapEnd -= numBytes;
    CopyMemory(pBuf->pGapEnd, pBuf->pCursor, numBytes);
    pBuf->pGapBegin = pBuf->pCursor;
  }
  else if (pBuf->pCursor > pBuf->pGapEnd)
  {
    size_t numBytes = pBuf->pCursor - pBuf->pGapEnd;
    CopyMemory(pBuf->pGapBegin, pBuf->pGapEnd, numBytes);
    pBuf->pGapEnd = pBuf->pCursor;
    pBuf->pGapBegin += numBytes;
  }
}

void mj::GapBufferIncrementCursor(GapBuffer* pBuf)
{
  if (pBuf->pCursor < (pBuf->pBufEnd - 1)) // Note: Skip last '\0' due to pasted text
  {
    if (pBuf->pCursor == pBuf->pGapBegin)
    {
      pBuf->pCursor = pBuf->pGapEnd + 1;
    }
    else
    {
      pBuf->pCursor++;
    }
    CursorUpdate(pBuf);
  }
}

void mj::GapBufferDecrementCursor(GapBuffer* pBuf)
{
  if (pBuf->pCursor > pBuf->pBufBegin) // Note: Skip last '\0' due to pasted text
  {
    if (pBuf->pCursor == pBuf->pGapEnd)
    {
      pBuf->pCursor = pBuf->pGapBegin - 1;
    }
    else
    {
      pBuf->pCursor--;
    }
    CursorUpdate(pBuf);
  }
}

void mj::GapBufferInit(GapBuffer* pBuf, void* pBegin, void* pEnd)
{
  pBuf->pBufBegin = (char*)pBegin;
  pBuf->pBufEnd   = (char*)pEnd;
  pBuf->pGapBegin = (char*)pBegin;
  pBuf->pGapEnd   = (char*)pEnd;
  pBuf->pCursor   = (char*)pBegin;
}

void mj::GapBufferSetText(GapBuffer* pBuf, const wchar_t* pText)
{
  int numBytes = mj::win32::Narrow(pBuf->pBufBegin, pText, -1, (int)(pBuf->pBufEnd - pBuf->pBufBegin));
  if (numBytes > 0)
  {
    pBuf->pGapBegin = pBuf->pBufBegin + numBytes;
    pBuf->pGapEnd   = pBuf->pBufEnd;
  }
}
