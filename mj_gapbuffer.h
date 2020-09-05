#pragma once
#include "mj_common.h"

namespace mj
{
  // Operates on UTF-8.
  class GapBuffer
  {
  private:
    /// <summary>
    /// Points to the first character in the buffer. Immutable.
    /// </summary>
    wchar_t* pBufBegin;
    /// <summary>
    /// Points to one past the last character in the buffer. Immutable.
    /// </summary>
    wchar_t* pBufEnd;
    /// <summary>
    /// Points to the first free character in the gap.
    /// </summary>
    wchar_t* pGapBegin;
    /// <summary>
    /// Points to one past the last free character in the gap.
    /// </summary>
    wchar_t* pGapEnd;
    wchar_t* pCaret;

    void MoveGapToCaret();
    void MoveGapBeginToCaret();
    void MoveGapEndToCaret();

  public:
    uint32_t GetVirtualCaretPosition() const;
    wchar_t* GetLeftPtr() const;
    int GetLeftLength() const;
    wchar_t* GetRightPtr() const;
    int GetRightLength() const;

    void JumpEndOfLine();
    void JumpStartOfLine();
    void InsertCharacterAtCaret(wchar_t c);
    bool IncrementCaret();
    bool DecrementCaret();
    void DeleteAtCaret();
    void BackspaceAtCaret();
    void Init(void* pBegin, void* pEnd);
    /// <summary>
    /// Copies the text to the buffer.
    /// </summary>
    /// <param name="pText"></param>
    void SetText(const wchar_t* pText);
    void SetCaretPosition(uint32_t caretPosition);
  };
} // namespace mj
