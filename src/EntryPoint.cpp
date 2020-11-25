#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "FloatMagic.h"

#ifdef TRACY_ENABLE
/// <summary>
/// CRT entry point (required by Tracy for Profile build configuration)
/// </summary>
/// <param name="hInstance"></param>
/// <param name="hPrevInstance"></param>
/// <param name="lpCmdLine"></param>
/// <param name="nShowCmd"></param>
/// <returns></returns>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  static_cast<void>(hInstance);
  static_cast<void>(hPrevInstance);
  static_cast<void>(lpCmdLine);
  static_cast<void>(nShowCmd);

  ::FloatMagicMain();

  return 0;
}
#else

/// <summary>
/// Default entry CRT-less point
/// </summary>
void CALLBACK WinMainCRTStartup()
{
  mj::FloatMagicMain();

  // Running without CRT requires a manual call to ExitProcess
  ::ExitProcess(0);
}
#endif
