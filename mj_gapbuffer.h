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
    size_t GetVirtualCursorPosition() const;
    char* GetLeftPtr() const;
    size_t GetLeftLength() const;
    char* GetRightPtr() const;
    size_t GetRightLength() const;

    void JumpEndOfLine();
    void JumpStartOfLine();
    void InsertCharacterAtCursor(wchar_t c);
    void IncrementCursor();
    void DecrementCursor();
    void DeleteAtCursor();
    void BackspaceAtCursor();
    void Init(void* pBegin, void* pEnd);
    void SetText(const wchar_t* pText);
  };
} // namespace mj
