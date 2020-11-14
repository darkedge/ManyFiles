#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"

#include "minicrt.h"

void ErrorExit(const wchar_t* lpszFunction) 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 
    mini_swprintf_s((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),TEXT("%s failed with error %d: %s"), lpszFunction, dw, lpMsgBuf); 
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw); 
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

    EndPaint(hwnd, &ps);
  }
    return 0;
  }

  return DefWindowProcW(hwnd, message, wParam, lParam);
}

void FloatMagicMain()
{
  BOOL b = HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0);
        ErrorExit(L"GetProcessId");

  // Register the window class.
  static constexpr const auto className = L"Hoi";

  WNDCLASS wc      = {};
  wc.lpfnWndProc   = WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = className;

  RegisterClassW(&wc);

  // Create the window.

  HWND hwnd = CreateWindowExW(0,                                                          // Optional window styles.
                              className,                                                  // Window class
                              L"Window Title",                                            // Window text
                              WS_OVERLAPPEDWINDOW,                                        // Window style
                              CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Size and position
                              nullptr,                                                    // Parent window
                              nullptr,                                                    // Menu
                              HINST_THISCOMPONENT,                                        // Instance handle
                              nullptr                                                     // Additional application data
  );

  ShowWindow(hwnd, SW_SHOW);

  // Run the message loop.

  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}
