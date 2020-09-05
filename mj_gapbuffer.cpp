#include "mj_gapbuffer.h"
#include "mj_win32.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <strsafe.h>

/// <summary>
/// Assumes the caret is currently on a grapheme boundary.
/// When done, the caret is placed at the start of the gap.
/// </summary>
void mj::GapBuffer::MoveGapToCaret()
{
  if (this->pCaret < this->pGapBegin)
  {
    // [...|...[ --> ___]...]
    size_t numChars = this->pGapBegin - this->pCaret;
    this->pGapEnd -= numChars;
    StringCchCopyNW(this->pGapEnd, numChars, this->pCaret, numChars);
    this->pGapBegin = this->pCaret;
  }
  else if (this->pCaret > this->pGapEnd)
  {
    // [...[___ <-- ]...|...]
    size_t numChars = this->pCaret - this->pGapEnd;
    StringCchCopyNW(this->pGapBegin, numChars, this->pGapEnd, numChars);
    this->pGapEnd = this->pCaret;
    this->pGapBegin += numChars;
  }
}

/// <summary>
/// Caret will point at first empty character in gap
/// </summary>
void mj::GapBuffer::MoveGapBeginToCaret()
{
  MoveGapToCaret();
}

/// <summary>
/// Caret will point at first character on right side
/// </summary>
void mj::GapBuffer::MoveGapEndToCaret()
{
  MoveGapToCaret();
  this->pCaret = this->pGapEnd;
}

uint32_t mj::GapBuffer::GetVirtualCaretPosition() const
{
  uint32_t position = 0;
  if (this->pCaret >= this->pGapEnd)
  {
    position += (uint32_t)(this->pCaret - this->pGapEnd);
    position += (uint32_t)(this->pGapBegin - this->pBufBegin);
  }
  else
  {
    position += (uint32_t)(this->pCaret - this->pBufBegin);
  }

  return position;
}

wchar_t* mj::GapBuffer::GetLeftPtr() const
{
  return this->pBufBegin;
}

int mj::GapBuffer::GetLeftLength() const
{
  return (int)(this->pGapBegin - this->pBufBegin);
}

wchar_t* mj::GapBuffer::GetRightPtr() const
{
  return this->pGapEnd;
}

int mj::GapBuffer::GetRightLength() const
{
  return (int)(this->pBufEnd - this->pGapEnd);
}

/// <summary>
/// Note: Does not work with multiple-code-point graphemes
/// </summary>
/// <param name="c"></param>
void mj::GapBuffer::InsertCharacterAtCaret(wchar_t c)
{
  this->MoveGapBeginToCaret();
  OutputDebugStringA("InsertCharacterAtCaret\n");
  *this->pCaret = c; // Points to free space
  this->pGapBegin = this->pCaret + 1;
  this->IncrementCaret();
}

void mj::GapBuffer::JumpStartOfLine()
{
#if 0
  OutputDebugStringA("JumpStartOfLine\n");
  while ((this->pCaret > this->pBufBegin)  //
         && (*(this->pCaret - 1) != '\n')  //
         && (*(this->pCaret - 1) != '\r')) //
  {
    this->pCaret--;
  }
#endif
}

void mj::GapBuffer::JumpEndOfLine()
{
#if 0
  OutputDebugStringA("JumpEndOfLine\n");
  while ((this->pCaret < this->pBufEnd)    //
         && (*(this->pCaret + 1) != '\0')  //
         && (*(this->pCaret + 1) != '\n')  //
         && (*(this->pCaret + 1) != '\r')) //
  {
    this->IncrementCaret();
  }
#endif
}

bool mj::GapBuffer::IncrementCaret()
{
  OutputDebugStringA("IncrementCaret\n");
  if (this->pCaret == this->pGapBegin)
  {
    this->pCaret = this->pGapEnd;
  }
  auto prev    = this->pCaret;
  this->pCaret = CharNextW(this->pCaret);
  return prev != this->pCaret;
}

bool mj::GapBuffer::DecrementCaret()
{
  if (this->pCaret > this->pBufBegin)
  {
    OutputDebugStringA("DecrementCaret\n");
    if (this->pCaret == this->pGapEnd)
    {
      this->pCaret = this->pGapBegin;
    }
    auto prev    = this->pCaret;
    this->pCaret = CharPrevW(this->pBufBegin, this->pCaret);
    return prev != this->pCaret;
  }

  return false;
}

void mj::GapBuffer::DeleteAtCaret()
{
  ptrdiff_t i = 1;
  if (this->pCaret == this->pGapBegin)
  {
    i = (this->pGapEnd - this->pCaret);
  }
  if (*(this->pCaret + i) != '\0')
  {
    OutputDebugStringA("DeleteAtCaret\n");
    this->MoveGapBeginToCaret();
    this->pGapEnd++;
    this->pCaret = this->pGapEnd;
  }
}

void mj::GapBuffer::BackspaceAtCaret()
{
#if 0
  // If caret is left of gap, caret must be > buffer begin
  // If caret is right of gap, gap begin must be > buffer begin
  if (((this->pGapBegin >= this->pCaret) && (this->pCaret > this->pBufBegin))      //
      || ((this->pGapBegin > this->pBufBegin) && (this->pCaret >= this->pGapEnd))) //
  {
    OutputDebugStringA("BackspaceAtCaret\n");
    this->MoveGapBeginToCaret();
    this->pCaret--;
    this->pGapBegin--;
  }
#endif
  this->MoveGapBeginToCaret();
  if (this->DecrementCaret())
  {
    this->pGapBegin = this->pCaret;
  }
}

void mj::GapBuffer::Init(void* pBegin, void* pEnd)
{
  this->pBufBegin = (wchar_t*)pBegin;
  this->pBufEnd   = (wchar_t*)pEnd;
  this->pGapBegin = (wchar_t*)pBegin;
  this->pGapEnd   = (wchar_t*)pEnd;
  this->pCaret    = (wchar_t*)pBegin;
}

void mj::GapBuffer::SetText(const wchar_t* pText)
{
  if (StringCchCopyW(this->pBufBegin, this->pBufEnd - this->pBufBegin, pText) == 0)
  {
    MJ_UNINITIALIZED size_t length;
    MJ_DISCARD(StringCchLengthW(pText, STRSAFE_MAX_CCH, &length));
    this->pGapBegin = this->pBufBegin + length; // + 1?
    this->pGapEnd   = this->pBufEnd;
  }
}

void mj::GapBuffer::SetCaretPosition(uint32_t caretPosition)
{
  // TODO
}
