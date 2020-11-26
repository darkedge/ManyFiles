#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace mj
{
  void FloatMagicMain();

  /// <summary>
  /// Main application. Is allocated in static memory (in the scope of the Main function).
  /// </summary>
  struct FloatMagic
  {
    HWND hWnd;
  };
} // namespace mj