#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"

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
  static_cast<void>(HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

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
