#pragma once

#define MJ_CONCAT(x, y)    x##y
#define MJ_XCONCAT(x, y)   MJ_CONCAT(x, y)
#define MJ_DEFER_DETAIL(x) MJ_XCONCAT(x, __COUNTER__)
#define MJ_DEFER(expr)     auto MJ_DEFER_DETAIL(_defer_) = mj::detail::defer([&]() { expr; })
#define MJ_NAMEOF(x)       MJ_CONCAT(L, #x)

#define STR(x)       #x
#define XSTR(x)      STR(x)
#define WSTR(x)      L##x
#define XWSTR(x)     WSTR(x)
#define WFILE        XWSTR(__FILE__)
#define __FILENAME__ WFILE

// Annotation macros

// Variable declaration does not call initialization code
#define MJ_UNINITIALIZED

#define MJ_COUNTOF(arr) sizeof(arr) / sizeof(arr[0])

// https://www.gingerbill.org/article/2015/08/19/defer-in-cpp/
namespace mj
{
  namespace detail
  {
    template <typename F>
    struct Defer
    {
      F f;
      Defer(F f) : f(f)
      {
      }
      ~Defer()
      {
        f();
      }
    };

    template <typename F>
    Defer<F> defer(F f)
    {
      return Defer<F>(f);
    }
  } // namespace detail
} // namespace mj
