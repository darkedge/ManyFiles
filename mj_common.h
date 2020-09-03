#pragma once
#include <stdint.h>
#include <new>

// Annotation macros

// Variable is uninitialized
#define MJ_UNINITIALIZED
// Argument is passed by reference
#define MJ_REF
// Return value is discarded
#define MJ_DISCARD(x) x

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// Generates a const-reference getter.
#define MJ_CRGETTER(name, member) \
  const decltype(member)& name() \
  { \
    return member; \
  }

namespace mj
{
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
    IStream() : end(nullptr), position(nullptr)
    {
    }

    IStream(void* begin, void* end) : end((char*)end), position((char*)begin)
    {
    }

    IStream(void* begin, size_t size) : end((char*)begin + size), position((char*)begin)
    {
    }

    IStream(const IStream& other) : end(other.end), position(other.position)
    {
    }

    IStream& operator=(const IStream& rhs)
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
    IStream& operator>>(T& t)
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
    IStream& Write(T& t)
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
    IStream& Read(T& t)
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

    IStream& Skip(size_t numBytes)
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
    void Fetch(T*&&) = delete;

    template <typename T>
    IStream& Fetch(T*& t)
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
