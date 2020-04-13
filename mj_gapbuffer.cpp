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
  pBuf->pCursor = pBuf->pGapBegin;
}

size_t mj::GapBufferGetVirtualCursorPosition(const GapBuffer* pBuf)
{
  size_t position = 0;
  if (pBuf->pCursor >= pBuf->pGapEnd)
  {
    position += (size_t)(pBuf->pCursor - pBuf->pGapEnd);
    position += (size_t)(pBuf->pGapBegin - pBuf->pBufBegin);
  }
  else
  {
    position += (size_t)(pBuf->pCursor - pBuf->pBufBegin);
  }

  return position;
}

void mj::GapBufferInsertCharacterAtCursor(GapBuffer* pBuf, wchar_t c)
{
  char buf[8];
  int numBytes = mj::win32::Narrow(buf, &c, 1, sizeof(buf));
  if (numBytes > 0)
  {
    CursorUpdate(pBuf);
    OutputDebugStringA("InsertCharacterAtCursor\n");
    CopyMemory(pBuf->pCursor, buf, numBytes);
    pBuf->pCursor += numBytes;
    pBuf->pGapBegin = pBuf->pCursor;
  }
}

void mj::GapBufferJumpStartOfLine(GapBuffer* pBuf)
{
  OutputDebugStringA("JumpStartOfLine\n");
  while ((pBuf->pCursor > pBuf->pBufBegin)  //
         && (*(pBuf->pCursor - 1) != '\n')  //
         && (*(pBuf->pCursor - 1) != '\r')) //
  {
    pBuf->pCursor--;
  }
}

static void IncrementCursorUnchecked(mj::GapBuffer* pBuf)
{
  if (pBuf->pCursor == pBuf->pGapBegin)
  {
    pBuf->pCursor = pBuf->pGapEnd + 1;
  }
  else
  {
    pBuf->pCursor++;
  }
}

void mj::GapBufferJumpEndOfLine(GapBuffer* pBuf)
{
  OutputDebugStringA("JumpEndOfLine\n");
  while ((pBuf->pCursor < pBuf->pBufEnd)    //
         && (*(pBuf->pCursor + 1) != '\0')  //
         && (*(pBuf->pCursor + 1) != '\n')  //
         && (*(pBuf->pCursor + 1) != '\r')) //
  {
    IncrementCursorUnchecked(pBuf);
  }
}

void mj::GapBufferIncrementCursor(GapBuffer* pBuf)
{
  if (pBuf->pCursor < pBuf->pBufEnd) // Note: Skip last '\0' due to pasted text
  {
    if (*(pBuf->pCursor + 1) != '\0')
    {
      OutputDebugStringA("IncrementCursor\n");
      IncrementCursorUnchecked(pBuf);
    }
  }
}

void mj::GapBufferDecrementCursor(GapBuffer* pBuf)
{
  if (pBuf->pCursor > pBuf->pBufBegin)
  {
    OutputDebugStringA("DecrementCursor\n");
    if (pBuf->pCursor == pBuf->pGapEnd)
    {
      pBuf->pCursor = pBuf->pGapBegin - 1;
    }
    else
    {
      pBuf->pCursor--;
    }
  }
}

void mj::GapBufferDeleteAtCursor(GapBuffer* pBuf)
{
  if (pBuf->pCursor < pBuf->pBufEnd) // Note: Skip last '\0' due to pasted text
  {
    if (*(pBuf->pCursor + 1) != '\0')
    {
      OutputDebugStringA("DeleteAtCursor\n");
      CursorUpdate(pBuf);
      pBuf->pGapEnd++;
      pBuf->pCursor = pBuf->pGapEnd;
    }
  }
}

void mj::GapBufferBackspaceAtCursor(GapBuffer* pBuf)
{
  // If cursor is left of gap, cursor must be > buffer begin
  // If cursor is right of gap, gap begin must be > buffer begin
  if (((pBuf->pGapBegin >= pBuf->pCursor) && (pBuf->pCursor > pBuf->pBufBegin))     //
      || ((pBuf->pGapBegin > pBuf->pBufBegin) && (pBuf->pCursor >= pBuf->pGapEnd))) //
  {
    OutputDebugStringA("BackspaceAtCursor\n");
    CursorUpdate(pBuf);
    pBuf->pCursor--;
    pBuf->pGapBegin--;
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
