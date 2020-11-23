#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

void FloatMagicMain();

namespace mj
{
  struct FloatMagic
  {
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    HWND pHwnd;
  };
} // namespace mj