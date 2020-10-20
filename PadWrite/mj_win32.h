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
  template <typename T>
  class ComPtr
  {
  public:
  protected:
    T* ptr;
    template <class U>
    friend class ComPtr;

    void InternalAddRef() const noexcept
    {
      if (this->ptr)
      {
        this->ptr->AddRef();
      }
    }

    unsigned long InternalRelease() noexcept
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
    ComPtr() noexcept : ptr(nullptr)
    {
    }

    ComPtr(decltype(__nullptr)) noexcept : ptr(nullptr)
    {
    }

    template <class U>
    ComPtr(U* other) noexcept : ptr(other)
    {
      InternalAddRef();
    }

    ComPtr(const ComPtr& other) noexcept : ptr(other.ptr)
    {
      InternalAddRef();
    }

    ComPtr(ComPtr&& other) noexcept : ptr(nullptr)
    {
      if (this != reinterpret_cast<ComPtr*>(&reinterpret_cast<unsigned char&>(other)))
      {
        Swap(other);
      }
    }

    ~ComPtr() noexcept
    {
      InternalRelease();
    }

    ComPtr& operator=(decltype(nullptr)) noexcept
    {
      InternalRelease();
      return *this;
    }

    ComPtr& operator=(T* other) noexcept
    {
      if (this->ptr != other)
      {
        ComPtr(other).Swap(*this);
      }
      return *this;
    }

    template <typename U>
    ComPtr& operator=(U* other) noexcept
    {
      ComPtr(other).Swap(*this);
      return *this;
    }

    ComPtr& operator=(const ComPtr& other) noexcept
    {
      if (this->ptr != other.ptr)
      {
        ComPtr(other).Swap(*this);
      }
      return *this;
    }

    template <class U>
    ComPtr& operator=(const ComPtr<U>& other) noexcept
    {
      ComPtr(other).Swap(*this);
      return *this;
    }

    ComPtr& operator=(ComPtr&& other) noexcept
    {
      ComPtr(static_cast<ComPtr&&>(other)).Swap(*this);
      return *this;
    }

    template <class U>
    ComPtr& operator=(ComPtr<U>&& other) noexcept
    {
      ComPtr(static_cast<ComPtr<U>&&>(other)).Swap(*this);
      return *this;
    }

    void Swap(ComPtr&& r) noexcept
    {
      T* tmp    = this->ptr;
      this->ptr = r.ptr;
      r.ptr     = tmp;
    }

    void Swap(ComPtr& r) noexcept
    {
      T* tmp    = this->ptr;
      this->ptr = r.ptr;
      r.ptr     = tmp;
    }

    operator bool() const noexcept
    {
      return this->ptr;
    }

    T* Get() const noexcept
    {
      return this->ptr;
    }

    T* operator->() const noexcept
    {
      return this->ptr;
    }

    T* const* GetAddressOf() const noexcept
    {
      return &this->ptr;
    }

    T** GetAddressOf() noexcept
    {
      return &this->ptr;
    }

    T** ReleaseAndGetAddressOf() noexcept
    {
      InternalRelease();
      return &this->ptr;
    }

    T* Detach() noexcept
    {
      T* ptr    = this->ptr;
      this->ptr = nullptr;
      return ptr;
    }

    void Attach(T* other) noexcept
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

    unsigned long Reset() noexcept
    {
      return InternalRelease();
    }

    HRESULT CopyTo(T** ptr) const noexcept
    {
      InternalAddRef();
      *ptr = this->ptr;
      return S_OK;
    }

    HRESULT CopyTo(REFIID riid, void** ptr) const noexcept
    {
      return this->ptr->QueryInterface(riid, ptr);
    }

    template <typename U>
    HRESULT CopyTo(U** ptr) const noexcept
    {
      return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(ptr));
    }

    // query for U interface
    template <typename U>
    HRESULT As(ComPtr<U>* p) const noexcept
    {
      return this->ptr->QueryInterface(__uuidof(U), reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }

    // query for riid interface and return as IUnknown
    HRESULT AsIID(REFIID riid, ComPtr<IUnknown>* p) const noexcept
    {
      return this->ptr->QueryInterface(riid, reinterpret_cast<void**>(p->ReleaseAndGetAddressOf()));
    }
  };
} // namespace mj
