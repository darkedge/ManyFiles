#pragma once
#include "mj_allocator.h"
#include "mj_macro.h"
#include <string.h>
#include <stdint.h>
#include <strsafe.h>

namespace mj
{
  size_t Kibibytes(size_t kib);
  size_t Mebibytes(size_t mib);
  size_t Gibibytes(size_t gib);
  size_t Tebibytes(size_t tib);

  /// <summary>
  /// Calculates min and max at the same time.
  /// Also checks if the order of a and b was initially correct or not.
  /// </summary>
  /// <typeparam name="T"></typeparam>
  /// <param name="a"></param>
  /// <param name="b"></param>
  /// <param name="outMin">Output min value.</param>
  /// <param name="outMax">Output max value.</param>
  /// <returns>True if a is min and b is max, otherwise false.</returns>
  template <typename T>
  inline constexpr bool minmax(const T& a, const T& b, T& outMin, T& outMax)
  {
    if (a < b)
    {
      outMin = a;
      outMax = b;
      return true;
    }
    else
    {
      outMin = b;
      outMax = a;
      return false;
    }
  }

  template <typename T>
  inline constexpr T min(const T& a, const T& b)
  {
    if (a < b)
    {
      return a;
    }
    else
    {
      return b;
    }
  }

  template <typename T>
  inline constexpr T max(const T& a, const T& b)
  {
    if (a > b)
    {
      return a;
    }
    else
    {
      return b;
    }
  }

  template <typename T>
  T clamp(const T& value, const T& minVal, const T& maxVal)
  {
    if (value < minVal)
    {
      return minVal;
    }
    else if (value > maxVal)
    {
      return maxVal;
    }
    return value;
  }

  template <typename T>
  void swap(T& a, T& b)
  {
    T c = a;
    a   = b;
    b   = c;
  }

  template <typename T>
  class ArrayListView;

  /// <summary>
  /// Similar to std::vector, except without constructors or destructors
  /// Requires explicit initialization and destruction.
  /// </summary>
  template <typename T>
  class ArrayList
  {
  private:
    static constexpr const size_t TSize = sizeof(T);

    AllocatorBase* pAllocator = nullptr;
    T* pData                  = nullptr; // Single allocation
    size_t numElements        = 0;
    size_t capacity           = 0;

  public:
    /// <summary>
    /// Does no allocation on construction.
    /// </summary>
    /// <returns></returns>
    void Init(AllocatorBase* pAllocator)
    {
      this->Destroy();
      this->pAllocator = pAllocator;
    }

    /// <summary>
    /// ArrayList with an initial capacity, allocated using the provided allocator.
    /// </summary>
    bool Init(AllocatorBase* pAllocator, size_t capacity)
    {
      this->Destroy();
      this->pAllocator = pAllocator;
      this->capacity   = capacity;
      this->pData      = reinterpret_cast<T*>(this->pAllocator->Allocate(capacity * TSize));
      return this->pData != nullptr;
    }

    /// <summary>
    /// Data is freed using the assigned allocator.
    /// </summary>
    void Destroy()
    {
      if (this->pAllocator && this->pData)
      {
        this->pAllocator->Free(this->pData);
      }
      this->pAllocator  = nullptr;
      this->pData       = nullptr;
      this->numElements = 0;
      this->capacity    = 0;
    }

    ArrayListView<T> CreateView()
    {
      return ArrayListView<T>(*this);
    }

    /// <summary>
    /// Checks if we can add more elements.
    /// Will attempt to allocate more memory if there isn't enough capacity
    /// and allocateIfNecessary is true (default).
    /// Does not increase element count.
    /// </summary>
    /// <param name="num">Amount of elements</param>
    /// <param name="allocateIfNecessary">If set to false, do not try to allocate more memory</param>
    /// <returns>True if there is memory for the specified amount of objects, otherwise false</returns>
    bool Reserve(size_t num, bool allocateIfNecessary = true)
    {
      if (num == 0)
      {
        return false;
      }

      if (numElements + num <= capacity)
      {
        return true;
      }
      else
      {
        if (allocateIfNecessary && Expand(numElements + num))
        {
          return Reserve(num);
        }
        else
        {
          return false;
        }
      }
    }

    /// <summary>
    /// Increases element count if successful.
    /// </summary>
    /// <param name="num"></param>
    /// <returns>Pointer to the newly reserved range, or nullptr if there is no more space.</returns>
    [[nodiscard]] T* Emplace(size_t num)
    {
      if (num == 0)
      {
        return nullptr;
      }

      if (numElements + num <= capacity)
      {
        T* ptr = pData + numElements;
        numElements += num;
        return ptr;
      }
      else
      {
        if (Expand(numElements + num))
        {
          return Emplace(num);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Inserts a range of elements at the specified index.
    /// </summary>
    /// <returns>A pointer to the start of the newly inserted elements, or nullptr if there was no space.</returns>
    [[nodiscard]] T* Insert(size_t pCurrent, const T* pSrc, size_t num)
    {
      // Note: These first checks are redundant after we recurse
      if (num == 0)
      {
        return this->end();
      }
      if (!pSrc)
      {
        return nullptr;
      }
      if (pCurrent > this->numElements) // Out of range
      {
        return nullptr;
      }

      if (this->numElements + num <= this->capacity)
      {
        // Number of elements to move right
        size_t numMove = this->numElements - pCurrent;

        T* ptr = this->begin() + pCurrent;

        // Move elements right
        static_cast<void>(::memmove(end() + num - numMove, ptr, numMove * TSize));

        // Copy new elements into index
        static_cast<void>(::memcpy(ptr, pSrc, num * TSize));

        this->numElements += num;

        return ptr;
      }
      else
      {
        if (this->Expand(this->numElements + num))
        {
          return this->Insert(pCurrent, pSrc, num);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Erases a range of elements.
    /// </summary>
    /// <param name="index">Index of the first element to be erased</param>
    /// <param name="num">Number of elements to erase</param>
    /// <returns>
    /// Pointer to the element at the specified index after erasure
    /// (can be the pEnd pointer if all trailing elements were erased)
    /// or nullptr if one or more arguments are out of range
    /// </returns>
    T* Erase(size_t index, size_t num)
    {
      if (index < this->numElements && num <= (this->numElements - index))
      {
        T* pEnd = this->end();
        this->numElements -= num;

        // Move elements from right of erased range to left
        T* pDst = begin() + index;
        T* pSrc = pDst + num;

        if (pSrc < pEnd)
        {
          memmove(pDst, pSrc, (pEnd - pDst - 1) * TSize);
          return pDst;
        }
      }

      return nullptr;
    }

    /// <summary>
    /// Assumes the source does not overlap this ArrayList.
    /// </summary>
    bool Assign(const T* pT, size_t length)
    {
      this->Destroy();

      if (this->Expand(length))
      {
        static_cast<void>(memcpy(this->pData, pT, length * TSize));
        this->numElements = length;
        return true;
      }
      else
      {
        return false;
      }
    }

    bool Contains(const T& t)
    {
      for (const auto& element : *this)
      {
        if (t == element)
        {
          return true;
        }
      }

      return false;
    }

    /// <summary>
    /// Adds a new element. Can trigger a reallocation.
    /// </summary>
    T* Add(const T& t)
    {
      if (this->numElements < this->capacity)
      {
        T* ptr = this->pData + this->numElements;
        *ptr   = t;
        ++this->numElements;
        return ptr;
      }
      else
      {
        if (this->Double())
        {
          return this->Add(t);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Removes all elements equal to the argument (using equality operator)
    /// </summary>
    void RemoveAll(const T& t)
    {
      T* ptr = this->pData;
      while (ptr && ptr != this->end())
      {
        if (*ptr == t)
        {
          ptr = this->Erase(ptr - pData, 1);
        }
        else
        {
          ptr++;
        }
      }
    }

    bool Copy(const ArrayList<T>& other)
    {
      // Check if the other array fits (allocate if necessary)
      if (this->Capacity() < other.Size() && !this->Reserve(other.Size() - this->Size()))
      {
        return false;
      }

      // Here, capacity >= numElements
      this->numElements = other.Size();
      static_cast<void>(::memcpy(this->pData, other.Get(), other.ByteWidth()));

      return true;
    }

    /// <summary>
    /// Sets number of elements to zero.
    /// Keeps current allocation.
    /// </summary>
    void Clear()
    {
      this->numElements = 0;
    }

    size_t Size() const
    {
      return this->numElements;
    }

    size_t Capacity() const
    {
      return this->capacity;
    }

    size_t ElemSize() const
    {
      return TSize;
    }

    size_t ByteWidth() const
    {
      return this->Size() * this->ElemSize();
    }

    const T* Get() const
    {
      return this->pData;
    }

    T* begin() const
    {
      return this->pData;
    }

    T* end() const
    {
      return this->pData + this->Size();
    }

    T& operator[](size_t index)
    {
      return this->pData[index];
    }

    operator mj::ArrayListView<T>()
    {
      return mj::ArrayListView(pData, numElements);
    }

  private:
    template <typename U>
    friend class ArrayListView;

    bool Double()
    {
      if (this->capacity == 0)
      {
        return this->Expand(4);
      }
      else
      {
        return this->Expand(2 * this->capacity);
      }
    }

    bool Expand(size_t newCapacity)
    {
      T* ptr = static_cast<T*>(this->pAllocator->Allocate(newCapacity * this->ElemSize()));

      if (ptr)
      {
        if (this->pData)
        {
          static_cast<void>(::memcpy(ptr, this->pData, this->numElements * this->ElemSize()));
          this->pAllocator->Free(this->pData);
        }
        this->capacity = newCapacity;
        this->pData    = ptr;
        return true;
      }
      else
      {
        // pData stays valid
        return false;
      }
    }
  };

  template <typename T>
  class ArrayListView
  {
  private:
    T* pData                            = nullptr;
    size_t numElements                  = 0;
    static constexpr const size_t TSize = sizeof(T);

  public:
    template <size_t Size>
    ArrayListView(T (&data)[Size]) : pData(data), numElements(Size)
    {
    }

    ArrayListView(T* pData, size_t numElements) : pData(pData), numElements(numElements)
    {
    }

    /// <summary>
    /// Creates a type-cast view of an ArrayList.
    /// sizeof(U) should be a proper multiple of TSize.
    /// </summary>
    /// <typeparam name="U"></typeparam>
    /// <param name="c"></param>
    /// <returns></returns>
    template <typename U>
    ArrayListView(ArrayList<U>& c) : pData((T*)c.pData), numElements(c.numElements * sizeof(U) / TSize)
    {
    }

    size_t Size() const
    {
      return numElements;
    }

    size_t ElemSize() const
    {
      return TSize;
    }

    size_t ByteWidth() const
    {
      return this->Size() * this->ElemSize();
    }

    T* Get() const
    {
      return this->pData;
    }

    T* begin() const
    {
      return this->pData;
    }

    T* end() const
    {
      return this->pData + this->Size();
    }

    T& operator[](size_t index)
    {
      return this->pData[index];
    }
  };

  /// <summary>
  /// Read/write stream wrapped around a memory buffer
  /// </summary>
  class MemoryBuffer
  {
  private:
    char* pEnd;
    char* pCurrent;

  public:
    MemoryBuffer() : pEnd(nullptr), pCurrent(nullptr)
    {
    }

    MemoryBuffer(void* pBegin, void* pEnd) : pEnd(static_cast<char*>(pEnd)), pCurrent(static_cast<char*>(pBegin))
    {
    }

    MemoryBuffer(void* pBegin, size_t size)
        : pEnd(static_cast<char*>(pBegin) + size), pCurrent(static_cast<char*>(pBegin))
    {
    }

    MemoryBuffer(const MemoryBuffer& other) : pEnd(other.pEnd), pCurrent(other.pCurrent)
    {
    }

    MemoryBuffer& operator=(const MemoryBuffer& rhs)
    {
      this->pEnd     = rhs.pEnd;
      this->pCurrent = rhs.pCurrent;
      return *this;
    }

    char* Position()
    {
      return this->pCurrent;
    }

    size_t SizeLeft()
    {
      return (this->pEnd - this->pCurrent);
    }

    template <typename T>
    MemoryBuffer& operator>>(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->pCurrent, sizeof(T));
        this->pCurrent += sizeof(T);
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Write(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(this->pCurrent, &t, sizeof(T));
        this->pCurrent += sizeof(T);
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Write(const void* pData, size_t size)
    {
      if (SizeLeft() >= size)
      {
        memcpy(this->pCurrent, pData, size);
        this->pCurrent += size;
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Read(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->pCurrent, sizeof(T));
        this->pCurrent += sizeof(T);
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Skip(size_t numBytes)
    {
      if (SizeLeft() >= numBytes)
      {
        this->pCurrent += numBytes;
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
      }
      return *this;
    }

    template <typename T>
    T* ReserveArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->pCurrent;
        this->pCurrent += (numElements * sizeof(T));
        return t;
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
        return nullptr;
      }
    }

    template <typename T, typename... Args>
    T* NewUnaligned(Args... args)
    {
      if (SizeLeft() >= sizeof(T))
      {
        T* t = (T*)this->pCurrent;
        this->pCurrent += sizeof(T);
        return new (t) T(args...);
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
        return nullptr;
      }
    }

    template <typename T>
    T* NewArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->pCurrent;
        this->pCurrent += (numElements * sizeof(T));
        return t;
      }
      else
      {
        this->pEnd     = nullptr;
        this->pCurrent = nullptr;
        return nullptr;
      }
    }

    bool Good()
    {
      return (this->pEnd && this->pCurrent);
    }
  };

  /// <summary>
  /// Does not free, allocates until full.
  /// </summary>
  class LinearAllocator : public AllocatorBase
  {
  private:
    MemoryBuffer memoryBuffer;

  public:
    void Init(const mj::Allocation& allocation)
    {
      this->memoryBuffer = MemoryBuffer(allocation.pAddress, allocation.numBytes);
    }

  protected:
    void* AllocateInternal(size_t numBytes) override
    {
      return this->memoryBuffer.NewArrayUnaligned<char>(numBytes);
    }

    void FreeInternal(void* ptr) override
    {
      static_cast<void>(ptr);
    }

    virtual const char* GetName() override
    {
      return STR(LinearAllocator);
    }
  };
} // namespace mj
