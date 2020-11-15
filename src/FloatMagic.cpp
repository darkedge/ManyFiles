#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Direct2D.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
  case WM_DESTROY:
    ::PostQuitMessage(0);
    return 0;

  case WM_PAINT:
  {
    MJ_UNINITIALIZED PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(hwnd, &ps);
    if (hdc)
    {
      // Don't care if it fails (returns zero, no GetLastError)
      static_cast<void>(::FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1)));

      // Always returns nonzero.
      static_cast<void>(::EndPaint(hwnd, &ps));
    }
    return 0;
  }
  default:
    break;
  }

  return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void FloatMagicMain()
{
  MJ_ERR_ZERO(::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));

  // Register the window class.
  static constexpr const auto className = L"Class Name";

  WNDCLASS wc      = {};
  wc.lpfnWndProc   = ::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = className;

  MJ_ERR_ZERO(::RegisterClassW(&wc));

  // Create the window.

  HWND hwnd = ::CreateWindowExW(0,                                                          // Optional window styles.
                                className,                                                  // Window class
                                L"Window Title",                                            // Window text
                                WS_OVERLAPPEDWINDOW,                                        // Window style
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Size and position
                                nullptr,                                                    // Parent window
                                nullptr,                                                    // Menu
                                HINST_THISCOMPONENT,                                        // Instance handle
                                nullptr // Additional application data
  );

  MJ_UNINITIALIZED mj::Direct2D direct2D;
  if (mj::Direct2DInit(hwnd, &direct2D))
  {
  }

  // If the window was previously visible, the return value is nonzero.
  // If the window was previously hidden, the return value is zero.
  static_cast<void>(::ShowWindow(hwnd, SW_SHOW));

  // Run the message loop.

  MSG msg = {};
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    static_cast<void>(::TranslateMessage(&msg));
    static_cast<void>(::DispatchMessageW(&msg));
  }
}
