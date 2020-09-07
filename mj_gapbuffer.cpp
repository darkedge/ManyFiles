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

wchar_t* mj::GapBuffer::PeekCaretPrev(wchar_t* pCaret) const
{
  const wchar_t* pCrt = pCaret;
  if (pCrt == this->pGapEnd)
  {
    pCrt = this->pGapBegin;
  }
  wchar_t* pPrev = CharPrevW(this->pBufBegin, pCrt);
  return pPrev == pCrt ? pCaret : pPrev;
}

wchar_t* mj::GapBuffer::PeekCaretNext(wchar_t* pCaret) const
{
  const wchar_t* pCrt = pCaret;
  if (pCrt == this->pGapBegin)
  {
    pCrt = this->pGapEnd;
  }
  wchar_t* pNext = CharNextW(pCrt);
  return pNext == pCrt ? pCaret : pNext;
}

wchar_t* mj::GapBuffer::GetPrevLineFeed(wchar_t* pCaret) const
{
  bool found = false;

  wchar_t* pPeek = pCaret;
  while (pPeek != this->pBufBegin)
  {
    wchar_t* pPrev = this->PeekCaretPrev(pPeek);
    if (pPrev == pPeek)
      return nullptr;

    if ((*pPrev == '\n') || (*pPrev == '\r'))
    {
      found = true;
    }
    else if (found)
    {
      return pPeek;
    }
    pPeek = pPrev;
  }

  return nullptr;
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

int mj::GapBuffer::JumpStartOfLine()
{
  int numShifts = 0;

  while (this->pCaret != this->pBufBegin)
  {
    wchar_t* pPrev = this->PeekCaretPrev(this->pCaret);
    if ((*pPrev == '\n') || (*pPrev == '\r'))
      break;
    this->pCaret = pPrev;
    numShifts++;
  }

  return numShifts;
}

int mj::GapBuffer::JumpEndOfLine()
{
  int numShifts = 0;
  wchar_t* pEnd = this->pGapEnd == this->pBufEnd ? this->pGapBegin : this->pBufEnd;

  while (this->pCaret != pEnd)
  {
    wchar_t* pNext = this->PeekCaretNext(this->pCaret);
    if ((*pNext == '\n') || (*pNext == '\r'))
      break;
    this->pCaret = pNext;
    numShifts++;
  }

  return numShifts;
}

bool mj::GapBuffer::IncrementCaret()
{
  wchar_t* pNext = this->pCaret;
  this->pCaret   = this->PeekCaretNext(this->pCaret);
  return pNext != this->pCaret;
}

bool mj::GapBuffer::DecrementCaret()
{
  wchar_t* pPrev = this->pCaret;
  this->pCaret   = this->PeekCaretPrev(this->pCaret);
  return pPrev != this->pCaret;
}

/// <summary>
/// TODO: This does not work with word wrapping!
/// </summary>
bool mj::GapBuffer::CaretLinePrev()
{
  wchar_t* pPrevLineFeed = this->GetPrevLineFeed(this->pCaret);
  if (pPrevLineFeed)
  {
    int numShifts = this->JumpStartOfLine();
    this->pCaret = pPrevLineFeed;
    MJ_DISCARD(this->JumpStartOfLine());
    for (int i = 0; i < numShifts; i++)
    {
      MJ_DISCARD(this->IncrementCaret());
      wchar_t* pNext = this->PeekCaretNext(this->pCaret);
      if ((*pNext == '\n') || (*pNext == '\r'))
      {
        break;
      }
    }
    return true;
  }

  return false;
}

bool mj::GapBuffer::CaretLineNext()
{
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
  // [...[   ]|...]
  // [[   ]..|....]
  // [....|..[   ]]
  if (caretPosition <= this->GetLeftLength())
  {
    this->pCaret = this->pBufBegin + caretPosition;
  }
}
