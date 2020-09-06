#include "mj_gapbuffer.h"
#include "mj_win32.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <strsafe.h>

/// <summary>
/// Caret will point at first empty character in gap
/// </summary>
void mj::GapBuffer::MoveGapBeginToCaret()
{
  if (this->pCaret < this->pGapBegin)
  {
    // [...|[... --> ___]...]
    size_t numChars = this->pGapBegin - this->pCaret;
    this->pGapEnd -= numChars;
    CopyMemory(this->pGapEnd, this->pCaret, numChars * sizeof(wchar_t));
    this->pGapBegin = this->pCaret;
  }
  else if (this->pCaret > this->pGapEnd)
  {
    // [...[___ <-- ...]|...]
    size_t numChars = this->pCaret - this->pGapEnd;
    CopyMemory(this->pGapBegin, this->pGapEnd, numChars * sizeof(wchar_t));
    this->pGapEnd = this->pCaret;
    this->pGapBegin += numChars;
    this->pCaret = this->pGapBegin;
  }
}

wchar_t* mj::GapBuffer::PeekCaretPrev() const
{
  wchar_t* pCrt = this->pCaret;
  if (pCrt == this->pGapEnd)
  {
    pCrt = this->pGapBegin;
  }
  wchar_t* pPrev = CharPrevW(this->pBufBegin, pCrt);
  return pPrev == pCrt ? this->pCaret : pPrev;
}

wchar_t* mj::GapBuffer::PeekCaretNext() const
{
  wchar_t* pCrt = this->pCaret;
  if (pCrt == this->pGapBegin)
  {
    pCrt = this->pGapEnd;
  }
  wchar_t* pNext = CharNextW(pCrt);
  return pNext == pCrt ? this->pCaret : pNext;
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

const wchar_t* mj::GapBuffer::GetLeftPtr() const
{
  return this->pBufBegin;
}

int mj::GapBuffer::GetLeftLength() const
{
  return (int)(this->pGapBegin - this->pBufBegin);
}

const wchar_t* mj::GapBuffer::GetRightPtr() const
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
  *this->pCaret   = c; // Points to free space
  this->pGapBegin = CharNextW(this->pCaret);
  MJ_DISCARD(this->IncrementCaret());
}

void mj::GapBuffer::JumpStartOfLine()
{
  while (this->pCaret != this->pBufBegin)
  {
    wchar_t* pPrev = this->PeekCaretPrev();
    if ((*pPrev == '\n') || (*pPrev == '\r'))
      break;
    this->pCaret = pPrev;
  }
}

void mj::GapBuffer::JumpEndOfLine()
{
  wchar_t* pEnd = this->pGapEnd == this->pBufEnd ? this->pGapBegin : this->pBufEnd;
  while (this->pCaret != pEnd)
  {
    wchar_t* pNext = this->PeekCaretNext();
    if ((*pNext == '\n') || (*pNext == '\r'))
      break;
    this->pCaret = pNext;
  }
}

bool mj::GapBuffer::IncrementCaret()
{
  wchar_t* pNext = this->pCaret;
  this->pCaret   = this->PeekCaretNext();
  return pNext != this->pCaret;
}

bool mj::GapBuffer::DecrementCaret()
{
  wchar_t* pPrev = this->pCaret;
  this->pCaret   = this->PeekCaretPrev();
  return pPrev != this->pCaret;
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
    this->MoveGapBeginToCaret();
    this->pGapEnd++;
    this->pCaret = this->pGapEnd;
  }
}

void mj::GapBuffer::BackspaceAtCaret()
{
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
    this->pGapBegin = this->pBufBegin + length;
    this->pGapEnd   = this->pBufEnd;
  }
}

void mj::GapBuffer::SetCaretPosition(uint32_t caretPosition)
{
  // TODO
}
