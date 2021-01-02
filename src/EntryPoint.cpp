#include "MainWindow.h"
#include "mj_win32.h"
#include "ErrorExit.h"
#include "mj_allocatordebug.h"

static void Main()
{
  mj::MainWindow pMainWindow;
  pMainWindow.Run();
}

#ifdef TRACY_ENABLE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

/// <summary>
/// CRT entry point (required by Tracy for Profile build configuration)
/// </summary>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  static_cast<void>(hInstance);
  static_cast<void>(hPrevInstance);
  static_cast<void>(lpCmdLine);
  static_cast<void>(nShowCmd);

  Main();

  return 0;
}

#else

/// <summary>
/// Default entry CRT-less point
/// </summary>
void CALLBACK WinMainCRTStartup()
{
  Main();

  // Running without CRT requires a manual call to ExitProcess
  ::ExitProcess(0);
}
#endif
