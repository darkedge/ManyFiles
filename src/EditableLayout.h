#pragma once
#include "mj_common.h"
#include "mj_win32.h"

struct MakeDWriteTextRange : public DWRITE_TEXT_RANGE
{
  inline MakeDWriteTextRange(UINT32 startPosition, UINT32 length)
  {
    this->startPosition = startPosition;
    this->length        = length;
  }

  // Overload to extend to end of text.
  inline MakeDWriteTextRange(UINT32 startPosition)
  {
    this->startPosition = startPosition;
    this->length        = UINT32_MAX - startPosition;
  }
};

class EditableLayout
{
public:
  struct CaretFormat
  {
    // The important range based properties for the current caret.
    // Note these are stored outside the layout, since the current caret
    // actually has a format, independent of the text it lies between.
    wchar_t fontFamilyName[100];
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    FLOAT fontSize;
    DWRITE_FONT_WEIGHT fontWeight;
    DWRITE_FONT_STRETCH fontStretch;
    DWRITE_FONT_STYLE fontStyle;
    UINT32 color;
    BOOL hasUnderline;
    BOOL hasStrikethrough;
  };

public:
  EditableLayout(IDWriteFactory* factory) : factory_(factory)
  {
  }

  ~EditableLayout()
  {
  }

public:
  IDWriteFactory* GetFactory()
  {
    return factory_;
  }

  /// Inserts a given string in the text layout's stored string at a certain text postion;
  HRESULT STDMETHODCALLTYPE InsertTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout, mj::ArrayList<wchar_t>& text, UINT32 position,
                                         WCHAR const* textToInsert, // [lengthToInsert]
                                         UINT32 textToInsertLength, CaretFormat* caretFormat = nullptr);

  /// Deletes a specified amount characters from the layout's stored string.
  HRESULT STDMETHODCALLTYPE RemoveTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout, mj::ArrayList<wchar_t>& text, UINT32 position,
                                         UINT32 lengthToRemove);

  HRESULT STDMETHODCALLTYPE Clear(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout, mj::ArrayList<wchar_t>& text);

private:
  HRESULT STDMETHODCALLTYPE RecreateLayout(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout, const mj::ArrayList<wchar_t>& text);

  static HRESULT CopyGlobalProperties(IDWriteTextLayout* oldLayout, IDWriteTextLayout* newLayout);

  static HRESULT CopyRangedProperties(IDWriteTextLayout* oldLayout, UINT32 startPos, UINT32 endPos,
                                   UINT32 newLayoutTextOffset, IDWriteTextLayout* newLayout,
                                   bool isOffsetNegative = false);

  static HRESULT CopySinglePropertyRange(IDWriteTextLayout* oldLayout, UINT32 startPosForOld, IDWriteTextLayout* newLayout,
                                      UINT32 startPosForNew, UINT32 length,
                                      EditableLayout::CaretFormat* caretFormat = nullptr);

public:
  IDWriteFactory* factory_;
};
