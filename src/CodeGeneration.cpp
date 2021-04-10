#include <Windows.h>

/// <summary>
/// Default entry CRT-less point
/// </summary>
void CALLBACK WinMainCRTStartup()
{
  HANDLE hStdout          = ::GetStdHandle(STD_OUTPUT_HANDLE);
  const char* lpszPrompt1 = "Hello World!";
  DWORD cWritten;
  if (!::WriteFile(hStdout,               // output handle
                   lpszPrompt1,           // prompt string
                   lstrlenA(lpszPrompt1), // string length
                   &cWritten,             // bytes written
                   NULL))                 // not overlapped
  {
    MessageBox(NULL, TEXT("WriteFile"), TEXT("Console Error"), MB_OK);
    ::ExitProcess(1);
  }

  // Running without CRT requires a manual call to ExitProcess
  ::ExitProcess(0);
}
