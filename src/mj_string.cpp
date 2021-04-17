#include "mj_string.h"
#include "ErrorExit.h"
#define STRSAFE_NO_CB_FUNCTIONS
#include <strsafe.h>

// The StringBuilder only adds a null terminator in the ToStringClosed() function.

static const wchar_t s_IntToWideChar[] = { L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7',
                                           L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F' };

void mj::StringView::Init(const wchar_t* pString, size_t numChars)
{
  this->ptr = pString;
  this->len = numChars;
}

void mj::StringView::Init(const wchar_t* pString)
{
  if (!pString)
  {
    // Convert to empty string
    pString = L"";
  }

  MJ_UNINITIALIZED size_t length;
  if (SUCCEEDED(::StringCchLengthW(pString, STRSAFE_MAX_CCH, &length)))
  {
    ptr = pString;
    len = length;
  }
}

bool mj::StringView::Equals(const wchar_t* pString) const
{
  MJ_UNINITIALIZED mj::StringView other;
  other.Init(pString);
  return this->len == other.len && memcmp(this->ptr, other.ptr, this->len * sizeof(wchar_t)) == 0;
}

bool mj::StringView::IsEmpty() const
{
  return this->len == 0;
}

bool mj::StringView::ParseNumber(uint32_t* pNumber) const
{
  const wchar_t* pC = this->ptr;
  *pNumber          = 0;
  for (size_t i = 0; i < this->len; i++)
  {
    *pNumber *= 10;
    if (*pC >= 0x30)
    {
      wchar_t digit = *pC - 0x30;
      if (digit < 10)
      {
        *pNumber += digit;
      }
      else
      {
        return false;
      }
    }
    pC++;
  }
  return false;
}

ptrdiff_t mj::StringView::FindLastOf(const wchar_t* pString) const
{
  ptrdiff_t index = -1;
  MJ_UNINITIALIZED StringView subString;
  subString.Init(pString);

  // FindLastOf makes no sense on empty strings
  if (this->IsEmpty() || subString.IsEmpty() || subString.len > this->len)
  {
    return index;
  }

  // Move backwards
  const wchar_t* pIt = this->ptr + this->len - subString.len;
  while (pIt >= this->ptr)
  {
    // Compare forwards
    const wchar_t* pBegin      = pIt;
    const wchar_t* pComparison = subString.ptr;
    bool match                 = false;

    while (*pBegin == *pComparison && pBegin - pIt < subString.len)
    {
      ++pBegin;
      ++pComparison;
      if (pBegin - pIt == subString.len)
      {
        match = true;
        index = pIt - this->ptr;
        break;
      }
    }

    if (match)
    {
      break;
    }

    --pIt;
  }

  return index;
}

void mj::StringAlloc::Init(const StringView& stringView, AllocatorBase* pAllocator, bool addNullTerminator)
{
  MJ_EXIT_NULL(pAllocator);

  this->Destroy(pAllocator);
  if (stringView.IsEmpty())
  {
    return;
  }

  size_t len = stringView.len;
  if (addNullTerminator)
  {
    len++;
  }

  this->ptr = static_cast<wchar_t*>(pAllocator->Allocate(len * sizeof(*stringView.ptr)));
  if (ptr)
  {
    ::CopyMemory(this->ptr, stringView.ptr, stringView.len * sizeof(*stringView.ptr));
    this->len = len;
    if (addNullTerminator)
    {
      this->ptr[len - 1] = L'\0';
    }
  }
  else
  {
    this->len = 0;
  }
}

void mj::StringAlloc::Destroy(AllocatorBase* pAllocator)
{
  if (this->ptr)
  {
    MJ_EXIT_NULL(pAllocator);
    pAllocator->Free(this->ptr);
  }

  this->ptr = nullptr;
  this->len = 0;
}

bool mj::StringAlloc::IsEmpty() const
{
  return this->len == 0;
}

void mj::StringBuilder::SetArrayList(ArrayList<wchar_t>* pArrayList)
{
  this->pArrayList = pArrayList;
}

void mj::StringBuilder::Clear()
{
  this->pArrayList->Clear();
}

mj::StringBuilder& mj::StringBuilder::Append(int32_t integer)
{
  // TODO: Do we actually ever print a null terminator?
  wchar_t buf[12]; // E.g. ['-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0']
  wchar_t* pEnd   = buf + MJ_COUNTOF(buf);
  wchar_t* pHead  = pEnd;
  bool isNegative = integer < 0;

  if (isNegative)
  {
    integer = -integer;
  }

  do
  {
    int32_t div = integer / 10;                      // Calculate next value
    --pHead;                                         // fill it backwards
    *pHead  = s_IntToWideChar[integer - (div * 10)]; // Calculate rest value
    integer = div;                                   // Decimal 10 conversion
  } while (integer);

  // Add sign
  if (isNegative)
  {
    --pHead;
    *pHead = '-';
  }

  ptrdiff_t numChars = pEnd - pHead;
  MJ_UNINITIALIZED StringView string;
  string.Init(pHead, numChars);
  return this->Append(string);
}

mj::StringBuilder& mj::StringBuilder::AppendHex32(uint32_t dw)
{
  wchar_t buf[8]; // E.g. ['-', '2', '1', '4', '7', '4', '8', '3', '6', '4', '8', '\0']
  wchar_t* pEnd  = buf + MJ_COUNTOF(buf);
  wchar_t* pHead = pEnd;

  for (int i = 0; i < 8; i++)
  {
    --pHead;
    *pHead = s_IntToWideChar[dw & 0x0F];
    dw >>= 4;
  }

  ptrdiff_t numChars = pEnd - pHead;
  MJ_UNINITIALIZED StringView string;
  string.Init(pHead, numChars);
  return this->Append(string);
}

mj::StringBuilder& mj::StringBuilder::Append(const StringView& string)
{
  wchar_t* pDest = this->pArrayList->Emplace(string.len);

  if (pDest)
  {
    // Use memcpy instead of strcpy to prevent copying the null terminator
    static_cast<void>(::memcpy(pDest, string.ptr, string.len * sizeof(wchar_t)));
  }

  return *this;
}

mj::StringBuilder& mj::StringBuilder::Append(const wchar_t* pStringLiteral)
{
  MJ_UNINITIALIZED size_t numChars;
  if (SUCCEEDED(::StringCchLengthW(pStringLiteral, STRSAFE_MAX_CCH, &numChars)))
  {
    MJ_UNINITIALIZED StringView string;
    string.Init(pStringLiteral, numChars);
    return this->Append(string);
  }

  return *this;
}

mj::StringBuilder& mj::StringBuilder::Indent(uint32_t numSpaces)
{
  MJ_UNINITIALIZED StringView sv;
  sv.Init(L" ");
  for (uint32_t i = 0; i < numSpaces; i++)
  {
    this->Append(sv);
  }

  return *this;
}

mj::StringView mj::StringBuilder::ToStringOpen()
{
  auto length = this->pArrayList->Size();
  MJ_UNINITIALIZED mj::StringView string;
  string.Init(this->pArrayList->begin(), length);
  return string;
}

mj::StringView mj::StringBuilder::ToStringClosed()
{
  // Make sure the string is null-terminated
  wchar_t* pLast = this->pArrayList->Emplace(1);
  if (pLast)
  {
    // Truncate
    *pLast = L'\0';
  }
  else
  {
    auto size = this->pArrayList->Size();
    if (size > 0)
    {
      (*this->pArrayList)[size - 1] = L'\0';
    }
  }

  MJ_UNINITIALIZED size_t length;
  MJ_ERR_HRESULT(::StringCchLengthW(this->pArrayList->Get(), STRSAFE_MAX_CCH, &length));

  MJ_UNINITIALIZED mj::StringView string;
  string.Init(this->pArrayList->begin(), length);
  return string;
}

void mj::StringCache::Init(AllocatorBase* pAllocator)
{
  this->Destroy();
  // Use the same allocator for both
  this->strings.Init(pAllocator);
  this->buffer.Init(pAllocator);
}

void mj::StringCache::Destroy()
{
  this->strings.Destroy();
  this->buffer.Destroy();
}

mj::ArrayListView<const mj::StringView> mj::StringCache::CreateView()
{
  return ArrayListView<const StringView>(*this);
}

bool mj::StringCache::Add(const wchar_t* pStringLiteral)
{
  MJ_UNINITIALIZED StringView string;
  string.Init(pStringLiteral);
  return this->Add(string);
}

bool mj::StringCache::Add(const StringView& string)
{
  // Store old buffer pointer to track reallocation
  // Note: We are assuming that ArrayList reallocation will always return a new pointer
  // This should hold because we never free the old memory before allocating the new memory
  // and the null address is already caught by ArrayList
  const wchar_t* pDataOld = this->buffer.Get();
  size_t destSize         = string.len + 1; // Include null terminator
  if (this->strings.Reserve(1) && this->buffer.Reserve(destSize))
  {
    // These pointers should always be valid after calling Reserve
    StringView* pString = this->strings.Emplace(1);
    wchar_t* pDest      = this->buffer.Emplace(destSize);

    // Update string objects if reallocation occurred
    auto pDataNew = this->buffer.Get();
    if (pDataOld != pDataNew)
    {
      for (auto& str : this->strings)
      {
        str.ptr = pDataNew;
        pDataNew += str.len + 1;
      }
    }

    // Copy the new string
    // StringCchCopy(Ex)W does not support source strings without a null terminator
    // MJ_ERR_HRESULT(::StringCchCopyW(pDest, destSize, string.ptr));
    ::memcpy(pDest, string.ptr, string.len * sizeof(wchar_t));
    // Add null terminator in case string was not null terminated
    pDest[string.len] = L'\0';

    // Initialize the new string object
    pString->ptr = pDest;
    pString->len = string.len;

    return true;
  }

  return false;
}

bool mj::StringCache::Copy(const StringCache& other)
{
  // Check if the other array fits (allocate if necessary)
  if (this->strings.Capacity() < other.strings.Size() &&
      !this->strings.Reserve(other.strings.Size() - this->strings.Size()))
  {
    return false;
  }

  if (this->buffer.Copy(other.buffer))
  {
    // Should always succeed
    this->strings.Copy(other.strings);

    // Update string addresses
    auto pDataNew = this->buffer.Get();
    for (auto& str : this->strings)
    {
      str.ptr = pDataNew;
      pDataNew += str.len + 1;
    }
  }

  return true;
}

void mj::StringCache::Clear()
{
  this->strings.Clear();
  this->buffer.Clear();
}

size_t mj::StringCache::Size() const
{
  return this->strings.Size();
}

size_t mj::StringCache::Capacity() const
{
  return this->strings.Capacity();
}

mj::StringView* mj::StringCache::begin() const
{
  return this->strings.begin();
}

mj::StringView* mj::StringCache::end() const
{
  return this->strings.end();
}

mj::StringView* mj::StringCache::operator[](size_t index)
{
  if (index < this->Size())
  {
    return &this->strings[index];
  }
  return nullptr;
}

mj::StringCache::operator mj::ArrayListView<const mj::StringView>()
{
  return mj::ArrayListView(this->strings.Get(), this->strings.Size());
}
