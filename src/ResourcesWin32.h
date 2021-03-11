#pragma once
#include "mj_win32.h"

namespace res
{
  namespace win32
  {
    struct ECursor
    {
      enum Enum {
        Arrow,
        Vertical,
        Horizontal,
        COUNT
      };
    };

    void Init();
    void SetCursor(ECursor::Enum cursor);
    void Destroy();
  }
} // namespace res
