#include "FloatMagic.h"
#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Direct2D.h"
#include "Threadpool.h"

static void mjCreateMenu(HWND pHwnd)
{
  HMENU hMenu = ::CreateMenu();
  MJ_ERR_NULL(hMenu);

  MJ_UNINITIALIZED HMENU hSubMenu;

  MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
  // AppendMenu(hSubMenu, MF_STRING, ID_QUIT_ITEM, L"&Quit");
  MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&File"));

  MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
  MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&Reports"));

  // Loads DLLs: TextShaping
  MJ_ERR_ZERO(::SetMenu(pHwnd, hMenu));
}

LRESULT CALLBACK mj::FloatMagic::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  FloatMagic* pMainWindow = reinterpret_cast<FloatMagic*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    // Copy the lpParam from CreateWindowEx to this window's user data
    CREATESTRUCT* pcs  = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow        = reinterpret_cast<FloatMagic*>(pcs->lpCreateParams);
    pMainWindow->pHwnd = hwnd;
    MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));

    ::mjCreateMenu(hwnd);

    mj::Direct2DInit(hwnd);
  }
    return DefWindowProcW(hwnd, message, wParam, lParam);

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
  case MJ_TASKEND:
  {
    mj::Task* pTask   = reinterpret_cast<mj::Task*>(wParam);
    mj::TaskEndFn pFn = reinterpret_cast<mj::TaskEndFn>(lParam);
    pFn(pTask);

    mj::ThreadpoolTaskFree(pTask);
  }
  break;
  default:
    break;
  }

  return ::DefWindowProcW(hwnd, message, wParam, lParam);
}

void FloatMagicMain()
{
  MJ_ERR_ZERO(::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));
  mj::ThreadpoolInit();

  static constexpr const auto className = L"Class Name";

  WNDCLASS wc      = {};
  wc.lpfnWndProc   = mj::FloatMagic::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = className;
  MJ_ERR_ZERO(::RegisterClassW(&wc));

  MJ_UNINITIALIZED mj::FloatMagic floatMagic;

  // Loads DLLs: uxtheme, combase, msctf, oleaut32
  HWND pHwnd = ::CreateWindowExW(0,                                                          // Optional window styles.
                                 className,                                                  // Window class
                                 L"Window Title",                                            // Window text
                                 WS_OVERLAPPEDWINDOW,                                        // Window style
                                 CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Size and position
                                 nullptr,                                                    // Parent window
                                 nullptr,                                                    // Menu
                                 HINST_THISCOMPONENT,                                        // Instance handle
                                 &floatMagic); // Additional application data
  MJ_ERR_ZERO(pHwnd);

  // If the window was previously visible, the return value is nonzero.
  // If the window was previously hidden, the return value is zero.
  static_cast<void>(::ShowWindow(pHwnd, SW_SHOW));

  // Run the message loop.
  MSG msg = {};
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    static_cast<void>(::TranslateMessage(&msg));
    static_cast<void>(::DispatchMessageW(&msg));
  }

  mj::ThreadpoolDestroy();
}
