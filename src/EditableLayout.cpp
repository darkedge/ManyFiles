#include "Common.h"
#include "mj_win32.h"
#include "EditableLayout.h"

HRESULT EditableLayout::RecreateLayout(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                       const mj::ArrayList<wchar_t>& text)
{
  // Recreates the internally held layout.

  HRESULT hr = S_OK;

  size_t length{};
  if (SUCCEEDED(hr))
  {
    hr = ::StringCchLengthW(text.begin(), text.Capacity(), &length);
  }

  mj::ComPtr<IDWriteTextLayout> newLayout;
  hr =
      factory_->CreateTextLayout(text.begin(), static_cast<UINT32>(length), currentLayout.Get(),
                                 currentLayout->GetMaxWidth(), currentLayout->GetMaxHeight(), newLayout.GetAddressOf());

  if (SUCCEEDED(hr))
  {
    currentLayout = newLayout;
  }

  return hr;
}

HRESULT EditableLayout::CopySinglePropertyRange(IDWriteTextLayout* oldLayout, UINT32 startPosForOld,
                                                IDWriteTextLayout* newLayout, UINT32 startPosForNew, UINT32 length,
                                                EditableLayout::CaretFormat* caretFormat)
{
  // Copies a single range of similar properties, from one old layout
  // to a new one.
  HRESULT hr = S_OK;

  DWRITE_TEXT_RANGE range = { startPosForNew, mj_min(length, UINT32_MAX - startPosForNew) };

  // font collection
  mj::ComPtr<IDWriteFontCollection> fontCollection;
  oldLayout->GetFontCollection(startPosForOld, fontCollection.GetAddressOf());
  newLayout->SetFontCollection(fontCollection.Get(), range);

  if (caretFormat)
  {
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontFamilyName(caretFormat->fontFamilyName, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetLocaleName(caretFormat->localeName, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontWeight(caretFormat->fontWeight, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontStyle(caretFormat->fontStyle, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontStretch(caretFormat->fontStretch, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontSize(caretFormat->fontSize, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetUnderline(caretFormat->hasUnderline, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetStrikethrough(caretFormat->hasStrikethrough, range);
    }
  }
  else
  {
    // font family
    MJ_UNINITIALIZED wchar_t fontFamilyName[100];
    fontFamilyName[0] = '\0';
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetFontFamilyName(startPosForOld, &fontFamilyName[0], ARRAYSIZE(fontFamilyName));
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontFamilyName(fontFamilyName, range);
    }

    // weight/width/slope
    DWRITE_FONT_WEIGHT weight   = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE style     = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STRETCH stretch = DWRITE_FONT_STRETCH_NORMAL;
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetFontWeight(startPosForOld, &weight);
    }
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetFontStyle(startPosForOld, &style);
    }
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetFontStretch(startPosForOld, &stretch);
    }

    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontWeight(weight, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontStyle(style, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontStretch(stretch, range);
    }

    // font size
    FLOAT fontSize{};
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetFontSize(startPosForOld, &fontSize);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetFontSize(fontSize, range);
    }

    // underline and strikethrough
    BOOL value{};
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetUnderline(startPosForOld, &value);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetUnderline(value, range);
    }
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetStrikethrough(startPosForOld, &value);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetStrikethrough(value, range);
    }

    // locale
    MJ_UNINITIALIZED wchar_t localeName[LOCALE_NAME_MAX_LENGTH];
    localeName[0] = '\0';
    if (SUCCEEDED(hr))
    {
      hr = oldLayout->GetLocaleName(startPosForOld, localeName, LOCALE_NAME_MAX_LENGTH);
    }
    if (SUCCEEDED(hr))
    {
      hr = newLayout->SetLocaleName(localeName, range);
    }
  }

  // drawing effect
  mj::ComPtr<IUnknown> drawingEffect;
  if (SUCCEEDED(hr))
  {
    hr = oldLayout->GetDrawingEffect(startPosForOld, drawingEffect.GetAddressOf());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetDrawingEffect(drawingEffect.Get(), range);
  }

  // inline object
  mj::ComPtr<IDWriteInlineObject> inlineObject;
  if (SUCCEEDED(hr))
  {
    hr = oldLayout->GetInlineObject(startPosForOld, inlineObject.GetAddressOf());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetInlineObject(inlineObject.Get(), range);
  }

  // typography
  mj::ComPtr<IDWriteTypography> typography;
  if (SUCCEEDED(hr))
  {
    hr = oldLayout->GetTypography(startPosForOld, typography.GetAddressOf());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetTypography(typography.Get(), range);
  }

  return hr;
}

UINT32 CalculateRangeLengthAt(IDWriteTextLayout* layout, UINT32 pos)
{
  // Determines the length of a block of similarly formatted properties.

  // Use the first getter to get the range to increment the current position.
  DWRITE_TEXT_RANGE incrementAmount = { pos, 1 };
  DWRITE_FONT_WEIGHT weight         = DWRITE_FONT_WEIGHT_NORMAL;

  // TODO MJ: Discarding possible error
  static_cast<void>(layout->GetFontWeight(pos, &weight, &incrementAmount));

  return incrementAmount.length - (pos - incrementAmount.startPosition);
}

/// <summary>
/// Copies properties that set on ranges.
/// </summary>
/// <param name="oldLayout"></param>
/// <param name="startPos"></param>
/// <param name="endPos"></param>
/// <param name="newLayoutTextOffset"></param>
/// <param name="newLayout"></param>
/// <param name="isOffsetNegative"></param>
/// <returns></returns>
HRESULT EditableLayout::CopyRangedProperties(IDWriteTextLayout* oldLayout, UINT32 startPos,
                                             UINT32 endPos, // an STL-like one-past position.
                                             UINT32 newLayoutTextOffset, IDWriteTextLayout* newLayout,
                                             bool isOffsetNegative)
{
  HRESULT hr = S_OK;

  UINT32 currentPos = startPos;
  while (currentPos < endPos)
  {
    UINT32 rangeLength = CalculateRangeLengthAt(oldLayout, currentPos);
    rangeLength        = mj_min(rangeLength, endPos - currentPos);
    if (isOffsetNegative)
    {
      if (SUCCEEDED(hr))
      {
        hr = CopySinglePropertyRange(oldLayout, currentPos, newLayout, currentPos - newLayoutTextOffset, rangeLength);
      }
    }
    else
    {
      if (SUCCEEDED(hr))
      {
        hr = CopySinglePropertyRange(oldLayout, currentPos, newLayout, currentPos + newLayoutTextOffset, rangeLength);
      }
    }
    currentPos += rangeLength;
  }

  return hr;
}

STDMETHODIMP EditableLayout::InsertTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                          mj::ArrayList<wchar_t>& text, UINT32 position,
                                          WCHAR const* textToInsert, // [lengthToInsert]
                                          UINT32 textToInsertLength, CaretFormat* caretFormat)
{
  HRESULT hr = S_OK;
  // Inserts text and shifts all formatting.

  // The inserted string gets all the properties of the character right before position.
  // If there is no text right before position, so use the properties of the character right after position.

  size_t length{};
  if (SUCCEEDED(hr))
  {
    hr = ::StringCchLengthW(text.begin(), text.Capacity(), &length);
  }

  // Copy all the old formatting.
  mj::ComPtr<IDWriteTextLayout> oldLayout(currentLayout);
  const UINT32 oldTextLength = static_cast<UINT32>(length);
  position                   = mj_min(position, static_cast<UINT32>(length));

  // Insert the new text and recreate the new text layout.
  static_cast<void>(text.Insert(position, textToInsert, textToInsertLength));

  if (SUCCEEDED(hr))
  {
    hr = RecreateLayout(currentLayout, text);
  }

  auto newLayout = currentLayout;

  if (SUCCEEDED(hr))
  {
    if (SUCCEEDED(hr))
    {
      hr = this->CopyGlobalProperties(oldLayout.Get(), newLayout.Get());
    }

    // For each property, get the position range and apply it to the old text.
    if (position == 0)
    {
      // Inserted text
      if (SUCCEEDED(hr))
      {
        hr = this->CopySinglePropertyRange(oldLayout.Get(), 0, newLayout.Get(), 0, textToInsertLength);
      }

      // The rest of the text
      if (SUCCEEDED(hr))
      {
        hr = this->CopyRangedProperties(oldLayout.Get(), 0, oldTextLength, textToInsertLength, newLayout.Get());
      }
    }
    else
    {
      // 1st block
      if (SUCCEEDED(hr))
      {
        hr = this->CopyRangedProperties(oldLayout.Get(), 0, position, 0, newLayout.Get());
      }

      // Inserted text
      if (SUCCEEDED(hr))
      {
        hr = this->CopySinglePropertyRange(oldLayout.Get(), position - 1, newLayout.Get(), position, textToInsertLength,
                                           caretFormat);
      }

      // Last block (if it exists)
      if (SUCCEEDED(hr))
      {
        hr = this->CopyRangedProperties(oldLayout.Get(), position, oldTextLength, textToInsertLength, newLayout.Get());
      }
    }

    // Copy trailing end.
    if (SUCCEEDED(hr))
    {
      hr = ::StringCchLengthW(text.begin(), text.Capacity(), &length);
    }
    if (SUCCEEDED(hr))
    {
      hr = this->CopySinglePropertyRange(oldLayout.Get(), oldTextLength, newLayout.Get(), static_cast<UINT32>(length),
                                         UINT32_MAX);
    }
  }

  return S_OK;
}

STDMETHODIMP EditableLayout::RemoveTextAt(IN OUT mj::ComPtr<IDWriteTextLayout>& currentLayout,
                                          mj::ArrayList<wchar_t>& text, UINT32 position, UINT32 lengthToRemove)
{
  // Removes text and shifts all formatting.
  HRESULT hr = S_OK;

  size_t length{};
  if (SUCCEEDED(hr))
  {
    hr = ::StringCchLengthW(text.begin(), text.Capacity(), &length);
  }

  // copy all the old formatting.
  mj::ComPtr<IDWriteTextLayout> oldLayout(currentLayout);
  const UINT32 oldTextLength = static_cast<UINT32>(length);

  // Remove the old text and recreate the new text layout.
  // TODO MJ: Check nullptr
  text.Erase(position, lengthToRemove);

  if (SUCCEEDED(hr))
  {
    hr = this->RecreateLayout(currentLayout, text);
  }

  auto newLayout = currentLayout;

  if (SUCCEEDED(hr))
  {
    hr = this->CopyGlobalProperties(oldLayout.Get(), newLayout.Get());
  }

  if (position == 0)
  {
    // The rest of the text
    if (SUCCEEDED(hr))
    {
      hr = this->CopyRangedProperties(oldLayout.Get(), lengthToRemove, oldTextLength, lengthToRemove, newLayout.Get(),
                                      true);
    }
  }
  else
  {
    // 1st block
    if (SUCCEEDED(hr))
    {
      hr = this->CopyRangedProperties(oldLayout.Get(), 0, position, 0, newLayout.Get(), true);
    }

    // Last block (if it exists, we increment past the deleted text)
    if (SUCCEEDED(hr))
    {
      hr = this->CopyRangedProperties(oldLayout.Get(), position + lengthToRemove, oldTextLength, lengthToRemove,
                                      newLayout.Get(), true);
    }
  }

  if (SUCCEEDED(hr))
  {
    hr = ::StringCchLengthW(text.begin(), text.Capacity(), &length);
  }

  if (SUCCEEDED(hr))
  {
    hr = this->CopySinglePropertyRange(oldLayout.Get(), oldTextLength, newLayout.Get(), static_cast<UINT32>(length),
                                       UINT32_MAX);
  }

  return hr;
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

HRESULT EditableLayout::CopyGlobalProperties(IDWriteTextLayout* oldLayout, IDWriteTextLayout* newLayout)
{
  // Copies global properties that are not range based.
  HRESULT hr = S_OK;

  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetTextAlignment(oldLayout->GetTextAlignment());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetParagraphAlignment(oldLayout->GetParagraphAlignment());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetWordWrapping(oldLayout->GetWordWrapping());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetReadingDirection(oldLayout->GetReadingDirection());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetFlowDirection(oldLayout->GetFlowDirection());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetIncrementalTabStop(oldLayout->GetIncrementalTabStop());
  }

  DWRITE_TRIMMING trimmingOptions = {};
  mj::ComPtr<IDWriteInlineObject> inlineObject;
  if (SUCCEEDED(hr))
  {
    hr = oldLayout->GetTrimming(&trimmingOptions, inlineObject.GetAddressOf());
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetTrimming(&trimmingOptions, inlineObject.Get());
  }

  DWRITE_LINE_SPACING_METHOD lineSpacingMethod = DWRITE_LINE_SPACING_METHOD_DEFAULT;
  FLOAT lineSpacing{}, baseline{};
  if (SUCCEEDED(hr))
  {
    hr = oldLayout->GetLineSpacing(&lineSpacingMethod, &lineSpacing, &baseline);
  }
  if (SUCCEEDED(hr))
  {
    hr = newLayout->SetLineSpacing(lineSpacingMethod, lineSpacing, baseline);
  }

  return hr;
}
