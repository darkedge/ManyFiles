#pragma once

namespace mj
{
  template <typename T>
  class optional
  {
  private:
    bool hasValue = false;
    T t;

  public:
    void Set(T t)
    {
      this->t        = t;
      this->hasValue = true;
    }

    void Reset()
    {
      hasValue = false;
    }

    bool TryGetValue(T* pT)
    {
      if (this->hasValue && pT)
      {
        *pT = t;
      }

      return this->hasValue;
    }

    // TODO: Perhaps also add a cast operator from optional<T> to T
    // to complement this assignment operator.
    optional<T>& operator=(const T& f)
    {
      this->t = f;
      this->hasValue = true;
      return *this;
    }
  };
} // namespace mj
