#pragma once
#include "mj_common.h"

namespace mj
{
  // Operates on UTF-8.
  class GapBuffer
  {
  private:
    char* pBufBegin;
    char* pBufEnd;
    char* pGapBegin;
    char* pGapEnd;
    char* pCursor;

    void CursorUpdate();
    void IncrementCursorUnchecked();

  public:
    uint32_t GetVirtualCursorPosition() const;
    char* GetLeftPtr() const;
    int GetLeftLength() const;
    char* GetRightPtr() const;
    int GetRightLength() const;

    void JumpEndOfLine();
    void JumpStartOfLine();
    void InsertCharacterAtCursor(wchar_t c);
    void IncrementCursor();
    void DecrementCursor();
    void DeleteAtCursor();
    void BackspaceAtCursor();
    void Init(void* pBegin, void* pEnd);
    void SetText(const wchar_t* pText);
    void SetCaretPosition(uint32_t caretPosition);
  };
} // namespace mj
