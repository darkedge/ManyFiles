#include "pch.h"
#include "MainWindow.h"
#include "mj_win32.h"
#include "ErrorExit.h"

static void Main()
{
  mj::MainWindow pMainWindow;
  pMainWindow.Run();
}

#ifdef TRACY_ENABLE

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
