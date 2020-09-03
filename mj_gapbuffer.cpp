#include "mj_gapbuffer.h"
#include "mj_win32.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

void mj::GapBuffer::CursorUpdate()
{
  if (this->pCursor < this->pGapBegin)
  {
    // Copy [pCursor...pGapBegin] to pGapEnd
    size_t numBytes = this->pGapBegin - this->pCursor;
    this->pGapEnd -= numBytes;
    CopyMemory(this->pGapEnd, this->pCursor, numBytes);
    this->pGapBegin = this->pCursor;
  }
  else if (this->pCursor > this->pGapEnd)
  {
    size_t numBytes = this->pCursor - this->pGapEnd;
    CopyMemory(this->pGapBegin, this->pGapEnd, numBytes);
    this->pGapEnd = this->pCursor;
    this->pGapBegin += numBytes;
  }
  this->pCursor = this->pGapBegin;
}

size_t mj::GapBuffer::GetVirtualCursorPosition() const
{
  size_t position = 0;
  if (this->pCursor >= this->pGapEnd)
  {
    position += (size_t)(this->pCursor - this->pGapEnd);
    position += (size_t)(this->pGapBegin - this->pBufBegin);
  }
  else
  {
    position += (size_t)(this->pCursor - this->pBufBegin);
  }

  return position;
}

void mj::GapBuffer::InsertCharacterAtCursor(wchar_t c)
{
  char buf[8];
  int numBytes = mj::win32::Narrow(buf, &c, 1, sizeof(buf));
  if (numBytes > 0)
  {
    this->CursorUpdate();
    OutputDebugStringA("InsertCharacterAtCursor\n");
    CopyMemory(this->pCursor, buf, numBytes);
    this->pCursor += numBytes;
    this->pGapBegin = this->pCursor;
  }
}

void mj::GapBuffer::JumpStartOfLine()
{
  OutputDebugStringA("JumpStartOfLine\n");
  while ((this->pCursor > this->pBufBegin)  //
         && (*(this->pCursor - 1) != '\n')  //
         && (*(this->pCursor - 1) != '\r')) //
  {
    this->pCursor--;
  }
}

void mj::GapBuffer::IncrementCursorUnchecked()
{
  if (this->pCursor == this->pGapBegin)
  {
    this->pCursor = this->pGapEnd + 1;
  }
  else
  {
    this->pCursor++;
  }
}

void mj::GapBuffer::JumpEndOfLine()
{
  OutputDebugStringA("JumpEndOfLine\n");
  while ((this->pCursor < this->pBufEnd)    //
         && (*(this->pCursor + 1) != '\0')  //
         && (*(this->pCursor + 1) != '\n')  //
         && (*(this->pCursor + 1) != '\r')) //
  {
    this->IncrementCursorUnchecked();
  }
}

void mj::GapBuffer::IncrementCursor()
{
  if (this->pCursor < this->pBufEnd) // Note: Skip last '\0' due to pasted text
  {
    if (*(this->pCursor + 1) != '\0')
    {
      OutputDebugStringA("IncrementCursor\n");
      this->IncrementCursorUnchecked();
    }
  }
}

void mj::GapBuffer::DecrementCursor()
{
  if (this->pCursor > this->pBufBegin)
  {
    OutputDebugStringA("DecrementCursor\n");
    if (this->pCursor == this->pGapEnd)
    {
      this->pCursor = this->pGapBegin - 1;
    }
    else
    {
      this->pCursor--;
    }
  }
}

void mj::GapBuffer::DeleteAtCursor()
{
  ptrdiff_t i = 1;
  if (this->pCursor == this->pGapBegin)
  {
    i = (this->pGapEnd - this->pCursor);
  }
  if (*(this->pCursor + i) != '\0')
  {
    OutputDebugStringA("DeleteAtCursor\n");
    this->CursorUpdate();
    this->pGapEnd++;
    this->pCursor = this->pGapEnd;
  }
}

void mj::GapBuffer::BackspaceAtCursor()
{
  // If cursor is left of gap, cursor must be > buffer begin
  // If cursor is right of gap, gap begin must be > buffer begin
  if (((this->pGapBegin >= this->pCursor) && (this->pCursor > this->pBufBegin))     //
      || ((this->pGapBegin > this->pBufBegin) && (this->pCursor >= this->pGapEnd))) //
  {
    OutputDebugStringA("BackspaceAtCursor\n");
    this->CursorUpdate();
    this->pCursor--;
    this->pGapBegin--;
  }
}

void mj::GapBuffer::Init(void* pBegin, void* pEnd)
{
  this->pBufBegin = (char*)pBegin;
  this->pBufEnd   = (char*)pEnd;
  this->pGapBegin = (char*)pBegin;
  this->pGapEnd   = (char*)pEnd;
  this->pCursor   = (char*)pBegin;
}

void mj::GapBuffer::SetText(const wchar_t* pText)
{
  int numBytes = mj::win32::Narrow(this->pBufBegin, pText, -1, (int)(this->pBufEnd - this->pBufBegin));
  if (numBytes > 0)
  {
    this->pGapBegin = this->pBufBegin + numBytes;
    this->pGapEnd   = this->pBufEnd;
  }
}
