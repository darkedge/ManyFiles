#pragma once
#include "mj_common.h"

namespace mj
{
  /// <summary>
  /// Wide string with known length
  /// </summary>
  struct String
  {
    const wchar_t* ptr = nullptr;
    size_t len         = 0; // Number of characters, compatible with DirectWrite "string length"
    String(const wchar_t* pString, size_t numChars);
    String(const wchar_t* pString);
  };

  class StringBuilder
  {
  private:
    ArrayList<wchar_t>* pArrayList;

  public:
    void SetArrayList(ArrayList<wchar_t>* pArrayList);

    StringBuilder& Append(const String& string);
    StringBuilder& Append(const wchar_t* pStringLiteral);
    StringBuilder& Append(int32_t integer);
    StringBuilder& AppendHex32(uint32_t dw);
    String ToString();
  };

  class StringCache
  {
  private:
    ArrayList<String> strings;
    ArrayList<wchar_t> buffer;

  public:
    /// <summary>
    /// Does no allocation on construction.
    /// </summary>
    /// <returns></returns>
    void Init(AllocatorBase* pAllocator);

    /// <summary>
    /// Data is freed using the assigned allocator.
    /// </summary>
    void Destroy();

    ArrayListView<const String> CreateView();

    /// <summary>
    /// Inserts a copy of this string into the buffer.
    /// </summary>
    /// <param name="pStringLiteral">The string to copy</param>
    /// <returns>
    /// True if adding was successful, otherwise false.
    /// Note: We do not return the pointer, as it might be invalidated as more strings are added.
    /// Use an iterator or the subscript operator to get the pointers afterward.
    /// </returns>
    bool Add(const wchar_t* pStringLiteral);

    bool Copy(const StringCache& other);

    void Clear();

    size_t Size() const;

    size_t Capacity() const;

    String* begin() const;

    String* end() const;

    String& operator[](size_t index);

    operator mj::ArrayListView<const String>();
  };
} // namespace mj
