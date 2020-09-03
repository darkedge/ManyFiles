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
    void JumpEndOfLine();
    void JumpStartOfLine();
    void InsertCharacterAtCursor(wchar_t c);
    void IncrementCursor();
    void DecrementCursor();
    void DeleteAtCursor();
    void BackspaceAtCursor();
    void Init(void* pBegin, void* pEnd);
    void SetText(const wchar_t* pText);

    MJ_GETTER(GetGapBegin, pGapBegin);
    MJ_GETTER(GetGapEnd, pGapEnd);
    MJ_GETTER(GetBufferBegin, pBufBegin);
    MJ_GETTER(GetBufferEnd, pBufEnd);
  };
} // namespace mj
