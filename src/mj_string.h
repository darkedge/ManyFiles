#pragma once
#include "mj_common.h"

namespace mj
{
  /// <summary>
  /// Wide string with known length
  /// </summary>
  struct StringView
  {
    MJ_UNINITIALIZED const wchar_t* ptr;
    MJ_UNINITIALIZED size_t len; // Number of characters, compatible with DirectWrite "string length"
    void Init(const wchar_t* pString, size_t numChars);
    void Init(const wchar_t* pString);
  };

  class StringBuilder
  {
  private:
    ArrayList<wchar_t>* pArrayList;

  public:
    void SetArrayList(ArrayList<wchar_t>* pArrayList);
    void Clear();

    // TODO: We have no way to report failure!
    StringBuilder& Append(const StringView& string);
    StringBuilder& Append(const wchar_t* pStringLiteral);
    StringBuilder& Append(int32_t integer);
    StringBuilder& AppendHex32(uint32_t dw);

    /// <summary>
    /// Does not add a null terminator.
    /// Additional appends to this StringBuilder are allowed.
    /// </summary>
    StringView ToStringOpen();

    /// <summary>
    /// Adds a null terminator to the string and returns.
    /// Note: Clear this StringBuilder before appending again!
    /// </summary>
    StringView ToStringClosed();
  };

  class StringCache
  {
  private:
    ArrayList<StringView> strings;
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

    ArrayListView<const StringView> CreateView();

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

    /// <summary>
    /// Adds a deep copy of a StringView to the buffer. The string does not need to be null-terminated.
    /// </summary>
    /// <returns>
    /// True if adding was successful, otherwise false.
    /// Note: We do not return the pointer, as it might be invalidated as more strings are added.
    /// Use an iterator or the subscript operator to get the pointers afterward.
    /// </returns>
    bool Add(const StringView& string);

    bool Copy(const StringCache& other);

    void Clear();

    size_t Size() const;

    size_t Capacity() const;

    StringView* begin() const;

    StringView* end() const;

    StringView& operator[](size_t index);

    operator mj::ArrayListView<const StringView>();
  };
} // namespace mj
