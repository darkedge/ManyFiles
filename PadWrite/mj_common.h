#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdint.h>

// Annotation macros

// Variable may be (partially) uninitialized
#define MJ_UNINITIALIZED
// Argument is passed by reference
#define MJ_REF
// Return value is discarded
#define MJ_DISCARD(x) (void)(x)

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

#define SAFE_RELEASE(_ptr) \
  if ((_ptr) != nullptr) \
  { \
    (_ptr)->Release(); \
    (_ptr) = nullptr; \
  }

// Raytracer resolution
static constexpr uint16_t MJ_RT_WIDTH  = 1600;
static constexpr uint16_t MJ_RT_HEIGHT = 1000;

// Window resolution
static constexpr uint16_t MJ_WND_WIDTH  = 1600;
static constexpr uint16_t MJ_WND_HEIGHT = 1000;

static constexpr uint16_t MJ_FS_WIDTH  = 1920;
static constexpr uint16_t MJ_FS_HEIGHT = 1080;

namespace mj
{
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
  void swap(T& a, T& b)
  {
    T c = a;
    a   = b;
    b   = c;
  }

#if 0
  template <typename T>
  class Array
  {
  public:
    template <uint32_t Size>
    Array(T(&data)[Size]) : pData(data), numElements(Size)
    {
    }

    uint32_t Size() const
    {
      return numElements;
    }

    uint32_t ElemSize() const
    {
      return sizeof(T);
    }

    uint32_t ByteWidth() const
    {
      return Size() * ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      assert(index < numElements);
      return pData[index];
    }

  private:
    T* pData;
    uint32_t numElements;
  };
#endif

  template <typename T>
  class ArrayListView;

  /// <summary>
  /// Reduced functionality mj::ArrayList replacement
  /// </summary>
  template <typename T>
  class ArrayList
  {
  public:
    ArrayList() : ArrayList(4)
    {
    }
    ArrayList(uint32_t capacity) : capacity(capacity)
    {

      pData =

          (T*)VirtualAlloc(0,                        //
                           capacity * sizeof(T),     //
                           MEM_COMMIT | MEM_RESERVE, //
                           PAGE_READWRITE);
    }
    ~ArrayList()
    {
      for (auto& t : *this)
      {
        t.~T();
      }
      VirtualFree(pData, 0, MEM_RELEASE);
    }

    /// <summary>
    ///
    /// </summary>
    /// <typeparam name="U"></typeparam>
    /// <returns></returns>
    template <typename U>
    ArrayListView<U> Cast()
    {
      return ArrayListView<U>(*this);
    }

    /// <summary>
    /// Reserves memory for multiple objects. No construction is done.
    /// </summary>
    /// <param name="num">Amount of objects</param>
    /// <returns>Null if there is no more space</returns>
    T* Reserve(uint32_t num)
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
        // TODO: Be smarter here
        if (Double())
        {
          return Reserve(num);
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Emplaces (constructs in-place) a new object.
    /// Uses placement-new. Could still be uninitialized!
    /// </summary>
    /// <returns>Null if there is no more space.</returns>
    template <typename... Ts>
    T* EmplaceSingle(Ts&&... args)
    {
      if (numElements < capacity)
      {
        T* ptr = pData + numElements;
        new (ptr) T(static_cast<Ts&&>(args)...);
        numElements++;
        return ptr;
      }
      else
      {
        if (Double())
        {
          return EmplaceSingle();
        }
        else
        {
          return nullptr;
        }
      }
    }

    /// <summary>
    /// Emplaces (constructs in-place) multiple objects.
    /// Uses placement-new.
    /// </summary>
    /// <returns>Null if there is no more space.</returns>
    template <typename... Ts>
    T* EmplaceMultiple(uint32_t num)
    {
      if (num == 0)
      {
        return nullptr;
      }

      if (numElements + num <= capacity)
      {
        T* ptr = (T*)operator new(num, pData + numElements);
        numElements += num;
        return ptr;
      }
      else
      {
        // TODO: Be smarter here
        if (Double())
        {
          return EmplaceMultiple(num);
        }
        else
        {
          return nullptr;
        }
      }
    }

#if 0
    /// <summary>
    /// Adds a copy of the argument to the end of the ArrayList.
    /// Uses copy constructor.
    /// </summary>
    /// <param name="t"></param>
    /// <returns>False if there is no more space.</returns>
    bool Add(const T& t)
    {
      if (numElements < capacity)
      {
        T* ptr = pData + numElements;
        new (ptr) T(t);
        numElements++;
        return ptr;
      }
      else
      {
        if (Double())
        {
          return Add(t);
        }
        else
        {
          return false;
        }
      }
    }
#endif

    void Clear()
    {
      for (auto& t : *this)
      {
        t.~T();
      }
      this->numElements = 0;
    }

    uint32_t Size() const
    {
      return numElements;
    }

    uint32_t ElemSize() const
    {
      return sizeof(T);
    }

    uint32_t ByteWidth() const
    {
      return Size() * ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      // assert(index < numElements);
      return pData[index];
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
      uint32_t newCapacity = 2 * capacity;
      // T* ptr               = (T*)realloc(pData, (size_t)newCapacity * ElemSize());
      T* ptr = (T*)VirtualAlloc(0,                                //
                                (size_t)newCapacity * ElemSize(), //
                                MEM_COMMIT | MEM_RESERVE,         //
                                PAGE_READWRITE);
      if (ptr)
      {
        CopyMemory(ptr, pData, (size_t)numElements * ElemSize());
        VirtualFree(pData, 0, MEM_RELEASE);
        capacity = newCapacity;
        pData    = ptr;
        return true;
      }
      else
      {
        // pData stays valid
        return false;
      }
    }

    T* pData             = nullptr;
    uint32_t numElements = 0;
    uint32_t capacity    = 4;
  };

  template <typename T>
  class ArrayListView
  {
  public:
    template <uint32_t Size>
    ArrayListView(T (&data)[Size]) : pData(data), numElements(Size)
    {
    }

    ArrayListView(T* pData, uint32_t numElements) : pData(pData), numElements(numElements)
    {
    }

    /// <summary>
    /// Creates a type-cast view of an ArrayList.
    /// sizeof(U) should be a proper multiple of sizeof(T).
    /// </summary>
    /// <typeparam name="U"></typeparam>
    /// <param name="c"></param>
    /// <returns></returns>
    template <typename U>
    ArrayListView(ArrayList<U>& c) : pData((T*)c.pData), numElements(c.numElements * sizeof(U) / sizeof(T))
    {
    }

    uint32_t Size() const
    {
      return numElements;
    }

    uint32_t ElemSize() const
    {
      return sizeof(T);
    }

    uint32_t ByteWidth() const
    {
      return Size() * ElemSize();
    }

    T* Get() const
    {
      return pData;
    }

    T* begin() const
    {
      return pData;
    }

    T* end() const
    {
      return pData + Size();
    }

    T& operator[](uint32_t index)
    {
      // assert(index < numElements);
      return pData[index];
    }

  private:
    T* pData             = nullptr;
    uint32_t numElements = 0;
  };

  /// <summary>
  /// Read/write stream wrapped around a memory buffer
  /// </summary>
  class MemoryBuffer
  {
  public:
    MemoryBuffer() : end(nullptr), position(nullptr)
    {
    }

    MemoryBuffer(void* pBegin, void* end) : end((char*)end), position((char*)pBegin)
    {
    }

    MemoryBuffer(void* pBegin, size_t size) : end((char*)pBegin + size), position((char*)pBegin)
    {
    }

    MemoryBuffer(const MemoryBuffer& other) : end(other.end), position(other.position)
    {
    }

    MemoryBuffer& operator=(const MemoryBuffer& rhs)
    {
      this->end      = rhs.end;
      this->position = rhs.position;
      return *this;
    }

    char* Position()
    {
      return this->position;
    }

    size_t SizeLeft()
    {
      return (this->end - this->position);
    }

    template <typename T>
    MemoryBuffer& operator>>(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Write(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(this->position, &t, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Write(void* pData, size_t size)
    {
      if (SizeLeft() >= size)
      {
        CopyMemory(this->position, pData, size);
        // memcpy(this->position, pData, size);
        this->position += size;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    MemoryBuffer& Read(T& t)
    {
      if (SizeLeft() >= sizeof(T))
      {
        memcpy(&t, this->position, sizeof(T));
        this->position += sizeof(T);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    MemoryBuffer& Skip(size_t numBytes)
    {
      if (SizeLeft() >= numBytes)
      {
        this->position += numBytes;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
      }
      return *this;
    }

    template <typename T>
    T* ReserveArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T, typename... Args>
    T* NewUnaligned(Args... args)
    {
      if (SizeLeft() >= sizeof(T))
      {
        T* t = (T*)this->position;
        this->position += sizeof(T);
        return new (t) T(args...);
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    template <typename T>
    T* NewArrayUnaligned(size_t numElements)
    {
      if (SizeLeft() >= (numElements * sizeof(T)))
      {
        T* t = (T*)this->position;
        this->position += (numElements * sizeof(T));
        for (size_t i = 0; i < numElements; i++)
        {
          new (t + i) T();
        }
        return t;
      }
      else
      {
        this->end      = nullptr;
        this->position = nullptr;
        return nullptr;
      }
    }

    bool Good()
    {
      return (this->end && this->position);
    }

  private:
    char* end;
    char* position;
  };
} // namespace mj
