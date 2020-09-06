#pragma once
#include "mj_common.h"

namespace mj
{
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

    void MoveGapBeginToCaret();
    [[nodiscard]] wchar_t* PeekCaretPrev(wchar_t* pCaret) const;
    [[nodiscard]] wchar_t* PeekCaretNext(wchar_t* pCaret) const;
    [[nodiscard]] wchar_t* GetPrevLineFeed(wchar_t* pCaret) const;

  public:
    [[nodiscard]] uint32_t GetVirtualCaretPosition() const;
    [[nodiscard]] const wchar_t* GetLeftPtr() const;
    [[nodiscard]] int GetLeftLength() const;
    [[nodiscard]] const wchar_t* GetRightPtr() const;
    [[nodiscard]] int GetRightLength() const;

    [[nodiscard]] int JumpEndOfLine();
    [[nodiscard]] int JumpStartOfLine();
    void InsertCharacterAtCaret(wchar_t c);
    [[nodiscard]] bool IncrementCaret();
    [[nodiscard]] bool DecrementCaret();
    [[nodiscard]] bool CaretLinePrev();
    [[nodiscard]] bool CaretLineNext();
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
