#pragma once
#include "mj_macro.h"

namespace mj
{
  namespace rng
  {
    class xoshiro128plusplus
    {
    private:
      MJ_UNINITIALIZED uint32_t s[4];

    public:
      void seed(uint32_t s0, uint32_t s1, uint32_t s2, uint32_t s3);
      uint32_t next();
      void jump();
      void long_jump();
    };
  } // namespace rng
} // namespace mj
