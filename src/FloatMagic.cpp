#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"

#define XWSTR(x)     WSTR(x)
#define WSTR(x)      L##x
#define WFILE        XWSTR(__FILE__)
#define __FILENAME__ (mini_wcsrchr("\\" WFILE, '\\') + 1)
#include "minicrt.h"

#define MJ_ERR_ZERO(expr)                              \
  do                                                   \
  {                                                    \
    HRESULT _hr = (expr);                              \
    if ((expr) == 0)                                   \
    {                                                  \
      ErrorExit(__FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                  \
  } while (0)

#define MJ_ERR_NONZERO(expr)                           \
  do                                                   \
  {                                                    \
    HRESULT _hr = (expr);                              \
    if ((expr) != 0)                                   \
    {                                                  \
      ErrorExit(__FILENAME__, __LINE__, XWSTR(#expr)); \
    }                                                  \
  } while (0)

void ErrorExit(const wchar_t* fileName, int lineNumber, const wchar_t* expression)
{
  // Retrieve the system error message for the last-error code

  MJ_UNINITIALIZED LPVOID lpMsgBuf;
  MJ_UNINITIALIZED LPVOID lpDisplayBuf;
  const DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
                dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, nullptr);

  // Display the error message and exit the process

  lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
                                    (lstrlenW((LPCWSTR)lpMsgBuf) + lstrlenW((LPCWSTR)expression) + 40) * sizeof(TCHAR));
  mini_swprintf_s((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                  TEXT("%s:%d - %s failed with error %d: %s"), //
                  fileName,                                    //
                  lineNumber,                                  //
                  expression,                                  //
                  dw,                                          //
                  lpMsgBuf);
  MessageBoxW(nullptr, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

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
  MJ_ERR_NONZERO(HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

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
