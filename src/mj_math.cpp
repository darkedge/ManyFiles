#include "pch.h"
#include "mj_math.h"
#define USE_SSE2
#include "sse_mathfun.h"

#define FORCE_EVAL(x)                               \
  do                                                \
  {                                                 \
    if constexpr (sizeof(x) == sizeof(float))       \
    {                                               \
      volatile float __x;                           \
      __x = (x);                                    \
    }                                               \
    else if constexpr (sizeof(x) == sizeof(double)) \
    {                                               \
      volatile double __x;                          \
      __x = (x);                                    \
    }                                               \
    else                                            \
    {                                               \
      volatile long double __x;                     \
      __x = (x);                                    \
    }                                               \
  } while (0)

float mj::cos(float x)
{
  float f[4];
  _mm_store_ps1(f, cos_ps(_mm_set_ps1(x)));
  return f[0];
}

float mj::sin(float x)
{
  float f[4];
  _mm_store_ps1(f, sin_ps(_mm_set_ps1(x)));
  return f[0];
}

float mj::floorf(float x)
{
  union {
    float f;
    uint32_t i;
  } u   = { x };
  const int e = static_cast<int>(u.i >> 23 & 0xff) - 0x7f;
  uint32_t m;

  if (e >= 23)
    return x;
  if (e >= 0)
  {
    m = 0x007fffff >> e;
    if ((u.i & m) == 0)
      return x;
    FORCE_EVAL(x + 0x1p120f);
    if (u.i >> 31)
      u.i += m;
    u.i &= ~m;
  }
  else
  {
    FORCE_EVAL(x + 0x1p120f);
    if (u.i >> 31 == 0)
      u.i = 0;
    else if (u.i << 1)
      u.f = -1.0;
  }
  return u.f;
}

float mj::abs(float x)
{
  return x < 0 ? -x : x;
}

static __inline unsigned __FLOAT_BITS(float __f)
{
  union {
    float __f;
    unsigned __i;
  } __u;
  __u.__f = __f;
  return __u.__i;
}

static __inline unsigned long long __DOUBLE_BITS(double __f)
{
  union {
    double __f;
    unsigned long long __i;
  } __u;
  __u.__f = __f;
  return __u.__i;
}

#define isnan(x)                                                            \
  (sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000 \
                              : sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL >> 1) > 0x7ffULL << 52 : true)

float mj::fmodf(float x, float y)
{
  union {
    float f;
    uint32_t i;
  } ux = { x }, uy = { y };
  int ex      = ux.i >> 23 & 0xff;
  int ey      = uy.i >> 23 & 0xff;
  const uint32_t sx = ux.i & 0x80000000;
  uint32_t i;
  uint32_t uxi = ux.i;

  if (uy.i << 1 == 0 || isnan(y) || ex == 0xff)
    return (x * y) / (x * y);
  if (uxi << 1 <= uy.i << 1)
  {
    if (uxi << 1 == uy.i << 1)
      return 0 * x;
    return x;
  }

  /* normalize x and y */
  if (!ex)
  {
    for (i = uxi << 9; i >> 31 == 0; ex--, i <<= 1)
      ;
    uxi <<= -ex + 1;
  }
  else
  {
    uxi &= -1U >> 9;
    uxi |= 1U << 23;
  }
  if (!ey)
  {
    for (i = uy.i << 9; i >> 31 == 0; ey--, i <<= 1)
      ;
    uy.i <<= -ey + 1;
  }
  else
  {
    uy.i &= -1U >> 9;
    uy.i |= 1U << 23;
  }

  /* x mod y */
  for (; ex > ey; ex--)
  {
    i = uxi - uy.i;
    if (i >> 31 == 0)
    {
      if (i == 0)
        return 0 * x;
      uxi = i;
    }
    uxi <<= 1;
  }
  i = uxi - uy.i;
  if (i >> 31 == 0)
  {
    if (i == 0)
      return 0 * x;
    uxi = i;
  }
  for (; uxi >> 23 == 0; uxi <<= 1, ex--)
    ;

  /* scale result up */
  if (ex > 0)
  {
    uxi -= 1U << 23;
    uxi |= (uint32_t)ex << 23;
  }
  else
  {
    uxi >>= -ex + 1;
  }
  uxi |= sx;
  ux.i = uxi;
  return ux.f;
}
