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
    bool Equals(const wchar_t* pString) const;
    bool IsEmpty() const;
    bool ParseNumber(uint32_t* pNumber) const;
    ptrdiff_t FindLastOf(const wchar_t* pString) const;
  };

  /// <summary>
  /// String with allocated storage.
  /// </summary>
  class StringAlloc
  {
  private:
    // We zero-initialize this, because we want Init to be safe.
    // Init will call Destroy beforehand just to be sure, which means that
    // Destroy will have to work with valid data.
    wchar_t* ptr = nullptr;
    size_t len   = 0; // Number of characters, compatible with DirectWrite "string length"

  public:
    void Init(const StringView& stringView, AllocatorBase* pAllocator, bool addNullTerminator = false);
    void Destroy(AllocatorBase* pAllocator);
    bool IsEmpty() const;
    wchar_t* Get() const
    {
      return this->ptr;
    }
    size_t Length() const
    {
      return this->len;
    }
  };

  class StaticStringBuilder;

  class StringBuilder
  {
  private:
    friend class StaticStringBuilder;
    ArrayList<wchar_t> arrayList;

  public:
    void Init(AllocatorBase* pAllocator);
    void Destroy();
    void Clear();

    // TODO: We have no way to report failure!
    StringBuilder& Append(const StringView& string);
    StringBuilder& Append(const wchar_t* pStringLiteral);
    StringBuilder& Append(int32_t integer);
    StringBuilder& AppendHex32(uint32_t dw);
    StringBuilder& Indent(uint32_t numSpaces);

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

  class StaticStringBuilder
  {
  private:
    StringBuilder sb;
    mj::LinearAllocator sbAlloc;

  public:
    void Init(const Allocation& allocation)
    {
      sbAlloc.Init(allocation);
      sb.arrayList.Init(&sbAlloc, allocation.numBytes / sizeof(wchar_t));
    }

    // Note: No Destroy() function as we rely on the allocation being freed afterwards.

    // clang-format off
    decltype(auto) Clear() { return sb.Clear(); }

    decltype(auto) Append(const StringView& string) { return sb.Append(string); }
    decltype(auto) Append(const wchar_t* pStringLiteral) { return sb.Append(pStringLiteral); }
    decltype(auto) Append(int32_t integer) { return sb.Append(integer); }
    decltype(auto) AppendHex32(uint32_t dw) { return sb.AppendHex32(dw); }
    decltype(auto) Indent(uint32_t numSpaces) { return sb.Indent(numSpaces); }

    decltype(auto) ToStringOpen() { return sb.ToStringOpen(); }
    decltype(auto) ToStringClosed()  { return sb.ToStringClosed(); }
    // clang-format on
  };

  /// <summary>
  /// The buffer contains null-terminated strings.
  /// </summary>
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

    void Pop();

    bool Copy(const StringCache& other);

    void Clear();

    size_t Size() const;

    size_t Capacity() const;

    StringView* begin() const;

    StringView* end() const;

    StringView* operator[](size_t index);

    operator mj::ArrayListView<const StringView>();
  };
} // namespace mj
