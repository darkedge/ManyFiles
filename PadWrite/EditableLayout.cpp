#include "Common.h"
#include "mj_win32.h"
#include "EditableLayout.h"

HRESULT EditableLayout::RecreateLayout(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                       const mj::ArrayList<wchar_t>& text)
{
  // Recreates the internally held layout.

  HRESULT hr = S_OK;

  mj::ComPtr<IDWriteTextLayout> newLayout;

  MJ_UNINITIALIZED size_t length;
  static_cast<void>(StringCchLengthW(text.begin(), text.Capacity(), &length));

  hr = factory_->CreateTextLayout(text.begin(), static_cast<UINT32>(length), currentLayout.Get(),
                                  currentLayout->GetMaxWidth(), currentLayout->GetMaxHeight(),
                                  newLayout.ReleaseAndGetAddressOf());

  if (SUCCEEDED(hr))
    currentLayout = newLayout;

  return hr;
}

void EditableLayout::CopySinglePropertyRange(IDWriteTextLayout* oldLayout, UINT32 startPosForOld,
                                             IDWriteTextLayout* newLayout, UINT32 startPosForNew, UINT32 length,
                                             EditableLayout::CaretFormat* caretFormat)
{
  // Copies a single range of similar properties, from one old layout
  // to a new one.

  DWRITE_TEXT_RANGE range = { startPosForNew, mj_min(length, UINT32_MAX - startPosForNew) };

  // font collection
  mj::ComPtr<IDWriteFontCollection> fontCollection;
  oldLayout->GetFontCollection(startPosForOld, fontCollection.GetAddressOf());
  newLayout->SetFontCollection(fontCollection.Get(), range);

  if (caretFormat)
  {
    newLayout->SetFontFamilyName(caretFormat->fontFamilyName, range);
    newLayout->SetLocaleName(caretFormat->localeName, range);
    newLayout->SetFontWeight(caretFormat->fontWeight, range);
    newLayout->SetFontStyle(caretFormat->fontStyle, range);
    newLayout->SetFontStretch(caretFormat->fontStretch, range);
    newLayout->SetFontSize(caretFormat->fontSize, range);
    newLayout->SetUnderline(caretFormat->hasUnderline, range);
    newLayout->SetStrikethrough(caretFormat->hasStrikethrough, range);
  }
  else
  {
    // font family
    wchar_t fontFamilyName[100];
    fontFamilyName[0] = '\0';
    oldLayout->GetFontFamilyName(startPosForOld, &fontFamilyName[0], ARRAYSIZE(fontFamilyName));
    newLayout->SetFontFamilyName(fontFamilyName, range);

    // weight/width/slope
    DWRITE_FONT_WEIGHT weight   = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE style     = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
    oldLayout->GetFontWeight(startPosForOld, &weight);
    oldLayout->GetFontStyle(startPosForOld, &style);
    oldLayout->GetFontStretch(startPosForOld, &stretch);

    newLayout->SetFontWeight(weight, range);
    newLayout->SetFontStyle(style, range);
    newLayout->SetFontStretch(stretch, range);

    // font size
    FLOAT fontSize = 12.0;
    oldLayout->GetFontSize(startPosForOld, &fontSize);
    newLayout->SetFontSize(fontSize, range);

    // underline and strikethrough
    BOOL value = FALSE;
    oldLayout->GetUnderline(startPosForOld, &value);
    newLayout->SetUnderline(value, range);
    oldLayout->GetStrikethrough(startPosForOld, &value);
    newLayout->SetStrikethrough(value, range);

    // locale
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    localeName[0] = '\0';
    oldLayout->GetLocaleName(startPosForOld, &localeName[0], ARRAYSIZE(localeName));
    newLayout->SetLocaleName(localeName, range);
  }

  // drawing effect
  mj::ComPtr<IUnknown> drawingEffect;
  oldLayout->GetDrawingEffect(startPosForOld, drawingEffect.GetAddressOf());
  newLayout->SetDrawingEffect(drawingEffect.Get(), range);

  // inline object
  mj::ComPtr<IDWriteInlineObject> inlineObject;
  oldLayout->GetInlineObject(startPosForOld, inlineObject.GetAddressOf());
  newLayout->SetInlineObject(inlineObject.Get(), range);

  // typography
  mj::ComPtr<IDWriteTypography> typography;
  oldLayout->GetTypography(startPosForOld, typography.GetAddressOf());
  newLayout->SetTypography(typography.Get(), range);
}

UINT32 CalculateRangeLengthAt(IDWriteTextLayout* layout, UINT32 pos)
{
  // Determines the length of a block of similarly formatted properties.

  // Use the first getter to get the range to increment the current position.
  DWRITE_TEXT_RANGE incrementAmount = { pos, 1 };
  DWRITE_FONT_WEIGHT weight         = DWRITE_FONT_WEIGHT_NORMAL;

  layout->GetFontWeight(pos, &weight, &incrementAmount);

  UINT32 rangeLength = incrementAmount.length - (pos - incrementAmount.startPosition);
  return rangeLength;
}

void EditableLayout::CopyRangedProperties(IDWriteTextLayout* oldLayout, UINT32 startPos,
                                          UINT32 endPos, // an STL-like one-past position.
                                          UINT32 newLayoutTextOffset, IDWriteTextLayout* newLayout,
                                          bool isOffsetNegative)
{
  // Copies properties that set on ranges.

  UINT32 currentPos = startPos;
  while (currentPos < endPos)
  {
    UINT32 rangeLength = CalculateRangeLengthAt(oldLayout, currentPos);
    rangeLength        = mj_min(rangeLength, endPos - currentPos);
    if (isOffsetNegative)
    {
      CopySinglePropertyRange(oldLayout, currentPos, newLayout, currentPos - newLayoutTextOffset, rangeLength);
    }
    else
    {
      CopySinglePropertyRange(oldLayout, currentPos, newLayout, currentPos + newLayoutTextOffset, rangeLength);
    }
    currentPos += rangeLength;
  }
}

STDMETHODIMP EditableLayout::InsertTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                          mj::ArrayList<wchar_t>& text, UINT32 position,
                                          WCHAR const* textToInsert, // [lengthToInsert]
                                          UINT32 textToInsertLength, CaretFormat* caretFormat)
{
  // Inserts text and shifts all formatting.

  HRESULT hr = S_OK;

  // The inserted string gets all the properties of the character right before position.
  // If there is no text right before position, so use the properties of the character right after position.

  MJ_UNINITIALIZED size_t length;
  static_cast<void>(StringCchLengthW(text.begin(), text.Capacity(), &length));

  // Copy all the old formatting.
  mj::ComPtr<IDWriteTextLayout> oldLayout(currentLayout);
  UINT32 oldTextLength = static_cast<UINT32>(length);
  position             = mj_min(position, static_cast<UINT32>(length));

  // Insert the new text and recreate the new text layout.
  text.Insert(position, textToInsert, textToInsertLength);

  if (SUCCEEDED(hr))
  {
    hr = RecreateLayout(currentLayout, text);
  }

  auto newLayout = currentLayout;

  if (SUCCEEDED(hr))
  {
    this->CopyGlobalProperties(oldLayout.Get(), newLayout.Get());

    // For each property, get the position range and apply it to the old text.
    if (position == 0)
    {
      // Inserted text
      this->CopySinglePropertyRange(oldLayout.Get(), 0, newLayout.Get(), 0, textToInsertLength);

      // The rest of the text
      this->CopyRangedProperties(oldLayout.Get(), 0, oldTextLength, textToInsertLength, newLayout.Get());
    }
    else
    {
      // 1st block
      this->CopyRangedProperties(oldLayout.Get(), 0, position, 0, newLayout.Get());

      // Inserted text
      this->CopySinglePropertyRange(oldLayout.Get(), position - 1, newLayout.Get(), position, textToInsertLength,
                                    caretFormat);

      // Last block (if it exists)
      this->CopyRangedProperties(oldLayout.Get(), position, oldTextLength, textToInsertLength, newLayout.Get());
    }

    // Copy trailing end.
    static_cast<void>(StringCchLengthW(text.begin(), text.Capacity(), &length));
    this->CopySinglePropertyRange(oldLayout.Get(), oldTextLength, newLayout.Get(), static_cast<UINT32>(length),
                                  UINT32_MAX);
  }

  return S_OK;
}

STDMETHODIMP EditableLayout::RemoveTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                          mj::ArrayList<wchar_t>& text, UINT32 position, UINT32 lengthToRemove)
{
  // Removes text and shifts all formatting.

  HRESULT hr = S_OK;

  MJ_UNINITIALIZED size_t length;
  static_cast<void>(StringCchLengthW(text.begin(), text.Capacity(), &length));

  // copy all the old formatting.
  mj::ComPtr<IDWriteTextLayout> oldLayout(currentLayout);
  UINT32 oldTextLength = static_cast<UINT32>(length);

  // Remove the old text and recreate the new text layout.
  static_cast<void>(text.Erase(position, lengthToRemove));

  if (SUCCEEDED(hr))
  {
    static_cast<void>(this->RecreateLayout(currentLayout, text));
  }

  auto newLayout = currentLayout;

  if (SUCCEEDED(hr))
  {
    this->CopyGlobalProperties(oldLayout.Get(), newLayout.Get());

    if (position == 0)
    {
      // The rest of the text
      this->CopyRangedProperties(oldLayout.Get(), lengthToRemove, oldTextLength, lengthToRemove, newLayout.Get(), true);
    }
    else
    {
      // 1st block
      this->CopyRangedProperties(oldLayout.Get(), 0, position, 0, newLayout.Get(), true);

      // Last block (if it exists, we increment past the deleted text)
      this->CopyRangedProperties(oldLayout.Get(), position + lengthToRemove, oldTextLength, lengthToRemove,
                                 newLayout.Get(), true);
    }

    static_cast<void>(StringCchLengthW(text.begin(), text.Capacity(), &length));

    this->CopySinglePropertyRange(oldLayout.Get(), oldTextLength, newLayout.Get(), static_cast<UINT32>(length),
                                  UINT32_MAX);
  }

  return S_OK;
}

STDMETHODIMP EditableLayout::Clear(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout, mj::ArrayList<wchar_t>& text)
{
  HRESULT hr = S_OK;

  text.Clear();
  static_cast<void>(text.Add('\0'));

  if (SUCCEEDED(hr))
  {
    hr = this->RecreateLayout(currentLayout, text);
  }

  return hr;
}

void EditableLayout::CopyGlobalProperties(IDWriteTextLayout* oldLayout, IDWriteTextLayout* newLayout)
{
  // Copies global properties that are not range based.

  static_cast<void>(newLayout->SetTextAlignment(oldLayout->GetTextAlignment()));
  static_cast<void>(newLayout->SetParagraphAlignment(oldLayout->GetParagraphAlignment()));
  static_cast<void>(newLayout->SetWordWrapping(oldLayout->GetWordWrapping()));
  static_cast<void>(newLayout->SetReadingDirection(oldLayout->GetReadingDirection()));
  static_cast<void>(newLayout->SetFlowDirection(oldLayout->GetFlowDirection()));
  static_cast<void>(newLayout->SetIncrementalTabStop(oldLayout->GetIncrementalTabStop()));

  DWRITE_TRIMMING trimmingOptions = {};
  mj::ComPtr<IDWriteInlineObject> inlineObject;
  static_cast<void>(oldLayout->GetTrimming(&trimmingOptions, inlineObject.ReleaseAndGetAddressOf()));
  static_cast<void>(newLayout->SetTrimming(&trimmingOptions, inlineObject.Get()));

  DWRITE_LINE_SPACING_METHOD lineSpacingMethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
  FLOAT lineSpacing                            = 0;
  FLOAT baseline                               = 0;
  static_cast<void>(oldLayout->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline));
  static_cast<void>(newLayout->SetLineSpacing(lineSpacingMethod, lineSpacing, baseline));
}
