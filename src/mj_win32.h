#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <unknwn.h>

// __ImageBase is better than GetCurrentModule()
// Can be cast to a HINSTANCE
#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

namespace mj
{
  // C-style cast fixes
  inline LPWSTR MakeIntResourceW(WORD i)
  {
    return reinterpret_cast<LPWSTR>(static_cast<ULONG_PTR>(i));
  }
  constexpr inline bool Succeeded(HRESULT hr)
  {
    return hr >= 0;
  }
  constexpr inline bool Failed(HRESULT hr)
  {
    return hr < 0;
  }
  constexpr inline WORD LoWord(DWORD_PTR l)
  {
    return static_cast<WORD>(l & 0xffff);
  }
  constexpr inline WORD HiWord(DWORD_PTR l)
  {
    return static_cast<WORD>((l >> 16) & 0xffff);
  }
  constexpr inline int GetXLParam(LPARAM lp)
  {
    return static_cast<int>(static_cast<short>(LoWord(lp)));
  }
  constexpr inline int GetYLParam(LPARAM lp)
  {
    return static_cast<int>(static_cast<short>(HiWord(lp)));
  }
  constexpr inline short GetWheelDeltaParam(WPARAM wParam)
  {
    return static_cast<short>(HiWord(wParam));
  }
  inline LPWSTR IdcArrow()
  {
    return MakeIntResourceW(32512);
  }
  inline LPWSTR IdcIBeam()
  {
    return MakeIntResourceW(32513);
  }
  constexpr int CwUseDefault = static_cast<int>(0x80000000);
  constexpr HRESULT kOK      = 0;

  template <typename T>
  class ComPtr
  {
  public:
  private:
    T* ptr;
    template <class U>
    friend class ComPtr;

    void InternalAddRef() const
    {
      if (this->ptr)
      {
        this->ptr->AddRef();
      }
    }

    unsigned long InternalRelease()
    {
      unsigned long ref = 0;
      T* temp           = this->ptr;

      if (temp)
      {
        this->ptr = nullptr;
        ref       = temp->Release();
      }

      return ref;
    }

  public:
    ComPtr() : ptr(nullptr)
    {
    }

    ComPtr(decltype(__nullptr)) : ptr(nullptr)
    {
    }

    template <class U>
    ComPtr(U* other) : ptr(other)
    {
      InternalAddRef();
    }

    ComPtr(const ComPtr& other) : ptr(other.ptr)
    {
      InternalAddRef();
    }

    ComPtr(ComPtr&& other) : ptr(nullptr)
    {
      if (this != reinterpret_cast<ComPtr*>(&reinterpret_cast<unsigned char&>(other)))
      {
        Swap(other);
      }
    }

    ~ComPtr()
    {
      InternalRelease();
    }

    ComPtr& operator=(decltype(nullptr))
    {
      InternalRelease();
      return *this;
    }

    ComPtr& operator=(T* other)
    {
      if (this->ptr != other)
      {
        ComPtr(other).Swap(*this);
      }
      return *this;
    }

    template <typename U>
    ComPtr& operator=(U* other)
    {
      ComPtr(other).Swap(*this);
      return *this;
    }

    ComPtr& operator=(const ComPtr& other)
    {
      if (this->ptr != other.ptr)
      {
        ComPtr(other).Swap(*this);
      }
      return *this;
    }

    template <class U>
    ComPtr& operator=(const ComPtr<U>& other)
    {
      ComPtr(other).Swap(*this);
      return *this;
    }

    ComPtr& operator=(ComPtr&& other)
    {
      ComPtr(static_cast<ComPtr&&>(other)).Swap(*this);
      return *this;
    }

    template <class U>
    ComPtr& operator=(ComPtr<U>&& other)
    {
      ComPtr(static_cast<ComPtr<U>&&>(other)).Swap(*this);
      return *this;
    }

    void Swap(ComPtr&& r)
    {
      T* tmp    = this->ptr;
      this->ptr = r.ptr;
      r.ptr     = tmp;
    }

    void Swap(ComPtr& r)
    {
      T* tmp    = this->ptr;
      this->ptr = r.ptr;
      r.ptr     = tmp;
    }

    operator bool() const
    {
      return this->ptr;
    }

    T* Get() const
    {
      return this->ptr;
    }

    T* operator->() const
    {
      return this->ptr;
    }

    T* const* GetAddressOf() const
    {
      return &this->ptr;
    }

    T** GetAddressOf()
    {
      return &this->ptr;
    }

    T** ReleaseAndGetAddressOf()
    {
      InternalRelease();
      return &this->ptr;
    }

    T* Detach()
    {
      T* ptr    = this->ptr;
      this->ptr = nullptr;
      return ptr;
    }

    void Attach(T* other)
    {
      if (this->ptr)
      {
        auto ref = this->ptr->Release();
        MJ_DISCARD(ref);
        // Attaching to the same object only works if duplicate references are being coalesced. Otherwise
        // re-attaching will cause the pointer to be released and may cause a crash on a subsequent dereference.
        assert(ref != 0 || this->ptr != other);
      }

      this->ptr = other;
    }

    unsigned long Reset()
    {
      return InternalRelease();
    }

    HRESULT CopyTo(T** ptr) const
    {
      InternalAddRef();
      *ptr = this->ptr;
      return S_OK;
    }

    HRESULT CopyTo(REFIID riid, void** ptr) const
    {
      return this->ptr->QueryInterface(riid, ptr);
    }

    template <typename U>
    HRESULT CopyTo(U** ptr) const
    {
      return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(ptr));
    }

    // query for U interface
    template <typename U>
    HRESULT As(ComPtr<U>* p) const
    {
      return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }

    // query for riid interface and return as IUnknown
    HRESULT AsIID(REFIID riid, ComPtr<IUnknown>* p) const
    {
      return this->ptr->QueryInterface(riid, reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }
  };
} // namespace mj
