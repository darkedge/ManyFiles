#include <stdint.h>
#define USE_SSE2
#include "sse_mathfun.h"

float cos(float x)
{
  float f[4];
  _mm_store_ps1(f, cos_ps(_mm_set_ps1(x)));
  return f[0];
}

float sin(float x)
{
  float f[4];
  _mm_store_ps1(f, sin_ps(_mm_set_ps1(x)));
  return f[0];
}

float floor(float x)
{
  int xi = (int)x;
  return x < xi ? xi - 1 : xi;
}

float abs(float x)
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
#define isnan(x) \
  (sizeof(x) == sizeof(float) ? (__FLOAT_BITS(x) & 0x7fffffff) > 0x7f800000 \
                              : sizeof(x) == sizeof(double) ? (__DOUBLE_BITS(x) & -1ULL >> 1) > 0x7ffULL << 52 : true)

double fmod(double x, double y)
{
  union {
    double f;
    uint64_t i;
  } ux = { x }, uy = { y };
  int ex = ux.i >> 52 & 0x7ff;
  int ey = uy.i >> 52 & 0x7ff;
  int sx = ux.i >> 63;
  uint64_t i;

  /* in the followings uxi should be ux.i, but then gcc wrongly adds */
  /* float load/store to inner loops ruining performance and code size */
  uint64_t uxi = ux.i;

  if (uy.i << 1 == 0 || isnan(y) || ex == 0x7ff)
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
    for (i = uxi << 12; i >> 63 == 0; ex--, i <<= 1)
      ;
    uxi <<= -ex + 1;
  }
  else
  {
    uxi &= -1ULL >> 12;
    uxi |= 1ULL << 52;
  }
  if (!ey)
  {
    for (i = uy.i << 12; i >> 63 == 0; ey--, i <<= 1)
      ;
    uy.i <<= -ey + 1;
  }
  else
  {
    uy.i &= -1ULL >> 12;
    uy.i |= 1ULL << 52;
  }

  /* x mod y */
  for (; ex > ey; ex--)
  {
    i = uxi - uy.i;
    if (i >> 63 == 0)
    {
      if (i == 0)
        return 0 * x;
      uxi = i;
    }
    uxi <<= 1;
  }
  i = uxi - uy.i;
  if (i >> 63 == 0)
  {
    if (i == 0)
      return 0 * x;
    uxi = i;
  }
  for (; uxi >> 52 == 0; uxi <<= 1, ex--)
    ;

  /* scale result */
  if (ex > 0)
  {
    uxi -= 1ULL << 52;
    uxi |= (uint64_t)ex << 52;
  }
  else
  {
    uxi >>= -ex + 1;
  }
  uxi |= (uint64_t)sx << 63;
  ux.i = uxi;
  return ux.f;
}
