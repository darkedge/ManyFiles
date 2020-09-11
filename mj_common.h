#pragma once
#include <stdint.h>
#include <new>

// Annotation macros

// Variable is uninitialized
#define MJ_UNINITIALIZED
// Argument is passed by reference
#define MJ_REF
// Return value is discarded
#define MJ_DISCARD(x) (void)x

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// Generates a const-reference getter.
#define MJ_CRGETTER(name, member) \
  const decltype(member)& name() const \
  { \
    return member; \
  }
// Generates a by-value getter.
#define MJ_GETTER(name, member) \
  decltype(member) name() const \
  { \
    return member; \
  }

namespace mj
{
  template <class T, size_t Size>
  class array
  {
  public:
    using value_type      = T;
    using size_type       = size_t;
    using difference_type = ptrdiff_t;
    using pointer         = T*;
    using const_pointer   = const T*;
    using reference       = T&;
    using const_reference = const T&;

    using iterator       = pointer;
    using const_iterator = const_pointer;

    [[nodiscard]] inline iterator begin() noexcept
    {
      return iterator(elements, 0);
    }

    [[nodiscard]] inline const_iterator begin() const noexcept
    {
      return const_iterator(elements, 0);
    }

    [[nodiscard]] inline iterator end() noexcept
    {
      return iterator(elements, Size);
    }

    [[nodiscard]] inline const_iterator end() const noexcept
    {
      return const_iterator(elements, Size);
    }

    [[nodiscard]] inline const_iterator cbegin() const noexcept
    {
      return begin();
    }

    [[nodiscard]] inline const_iterator cend() const noexcept
    {
      return end();
    }

    [[nodiscard]] constexpr size_type size() const noexcept
    {
      return Size;
    }

    [[nodiscard]] constexpr size_type max_size() const noexcept
    {
      return Size;
    }

    [[nodiscard]] constexpr bool empty() const noexcept
    {
      return false;
    }

    [[nodiscard]] inline reference operator[](_In_range_(0, Size - 1) size_type index) noexcept
    {
      return elements[index];
    }

    [[nodiscard]] constexpr const_reference operator[](_In_range_(0, Size - 1) size_type index) const noexcept
    {
      return elements[index];
    }

    [[nodiscard]] inline reference front() noexcept
    {
      return elements[0];
    }

    [[nodiscard]] constexpr const_reference front() const noexcept
    {
      return elements[0];
    }

    [[nodiscard]] inline reference back() noexcept
    {
      return elements[Size - 1];
    }

    [[nodiscard]] constexpr const_reference back() const noexcept
    {
      return elements[Size - 1];
    }

    [[nodiscard]] inline T* data() noexcept
    {
      return elements;
    }

    [[nodiscard]] inline const T* data() const noexcept
    {
      return elements;
    }

    T elements[Size];
  };

  template <typename T>
  void swap(T& a, T& b)
  {
    T c = a;
    a   = b;
    b   = c;
  }

  // stream for reading
  class IStream
  {
  public:
    IStream() noexcept : end(nullptr), position(nullptr)
    {
    }

    IStream(void* begin, void* end) noexcept : end((char*)end), position((char*)begin)
    {
    }

    IStream(void* begin, size_t size) noexcept : end((char*)begin + size), position((char*)begin)
    {
    }

    IStream(const IStream& other) noexcept : end(other.end), position(other.position)
    {
    }

    IStream& operator=(const IStream& rhs) noexcept
    {
      this->end      = rhs.end;
      this->position = rhs.position;
      return *this;
    }

    [[nodiscard]] char* Position() noexcept
    {
      return this->position;
    }

    [[nodiscard]] size_t SizeLeft() noexcept
    {
      return (this->end - this->position);
    }

    template <typename T>
    IStream& operator>>(T& t) noexcept
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
    IStream& Write(T& t) noexcept
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

    template <typename T>
    IStream& Read(T& t) noexcept
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

    IStream& Skip(size_t numBytes) noexcept
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
    void Fetch(T*&&) noexcept = delete;

    template <typename T>
    IStream& Fetch(T*& t) noexcept
    {
      if (SizeLeft() >= sizeof(T))
      {
        t = (T*)this->position;
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
    [[nodiscard]] T* ReserveArrayUnaligned(size_t numElements) noexcept
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
    [[nodiscard]] T* NewUnaligned(Args... args) noexcept
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
    [[nodiscard]] T* NewArrayUnaligned(size_t numElements) noexcept
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

    [[nodiscard]] bool Good() noexcept
    {
      return (this->end && this->position);
    }

  private:
    char* end;
    char* position;
  };
} // namespace mj
