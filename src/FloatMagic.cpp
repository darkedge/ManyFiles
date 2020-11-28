#include "FloatMagic.h"
#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Direct2D.h"
#include "Threadpool.h"
#include "HexEdit.h"
#include "File.h"
#include <shellapi.h> // CommandLineToArgvW
#include <strsafe.h>
#include <ShlObj.h> // File dialog
#include "minicrt.h"

static constexpr WORD ID_FILE_OPEN = 40001;

static wchar_t s_pArg[MAX_PATH + 1];

static HWND s_MainWindowHandle;

HWND mj::GetMainWindowHandle()
{
  return s_MainWindowHandle;
}

wchar_t* mj::GetCommandLineArgument()
{
  return s_pArg;
}

namespace mj
{
  static void CreateMenu(HWND hWnd)
  {
    HMENU hMenu = ::CreateMenu();
    MJ_ERR_NULL(hMenu);

    MJ_UNINITIALIZED HMENU hSubMenu;
    MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
    ::AppendMenuW(hSubMenu, MF_STRING, ID_FILE_OPEN, L"&Open...\tCtrl+O");
    MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&File"));

    MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
    MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&Help"));

    // Loads DLLs: TextShaping
    MJ_ERR_ZERO(::SetMenu(hWnd, hMenu));
  }

  void OpenFileDialog(FloatMagic* pFloatMagic)
  {
    ZoneScoped;

    MJ_UNINITIALIZED ::IFileOpenDialog* pFileOpen;
    MJ_ERR_HRESULT(::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                      reinterpret_cast<LPVOID*>(&pFileOpen)));

    if (SUCCEEDED(pFileOpen->Show(pFloatMagic->hWnd))) // Dialog result: User picked a file
    {
      MJ_UNINITIALIZED ::IShellItem* pItem;
      MJ_ERR_HRESULT(pFileOpen->GetResult(&pItem));

      MJ_UNINITIALIZED PWSTR pFileName;
      MJ_ERR_HRESULT(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pFileName));

      mj::LoadFileAsync(pFileName);

      ::CoTaskMemFree(pFileName);

      pItem->Release();
    }

    pFileOpen->Release();
  }

  /// <summary>
  /// Processes WM_COMMAND messages from the main WindowProc further.
  /// </summary>
  /// <param name="commandId"></param>
  void WindowProcCommand(FloatMagic* pFloatMagic, WORD commandId)
  {
    ZoneScoped;

    switch (commandId)
    {
    case ID_FILE_OPEN:
      mj::OpenFileDialog(pFloatMagic);
      break;
    }
  }

  LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    mj::FloatMagic* pMainWindow = reinterpret_cast<mj::FloatMagic*>(::GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_CREATE:
    {
      // Copy the lpParam from CreateWindowEx to this window's user data
      CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
      pMainWindow       = reinterpret_cast<mj::FloatMagic*>(pcs->lpCreateParams);
      pMainWindow->hWnd = hwnd;
      MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));

      mj::CreateMenu(hwnd);
      mj::Direct2DInit(hwnd);
      mj::LoadFileAsync(mj::GetCommandLineArgument());
    }
      return ::DefWindowProcW(hwnd, message, wParam, lParam);
    case WM_SIZE:
      mj::HexEditOnSize(HIWORD(lParam));
      return 0;
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
      pFn(pTask->pContext);

      mj::ThreadpoolTaskFree(pTask);
    }
    break;
    case WM_COMMAND:
      mj::WindowProcCommand(pMainWindow, LOWORD(wParam));
      break;
    case WM_VSCROLL:
      mj::HexEditOnScroll(LOWORD(wParam));
      break;
    default:
      break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
  }
} // namespace mj

void mj::FloatMagicMain()
{
  // Parse command line
  LPWSTR pCommandLine = ::GetCommandLineW();
  MJ_UNINITIALIZED int numArgs;
  MJ_UNINITIALIZED LPWSTR* ppArgs;
  // Return value must be freed using LocalFree
  MJ_ERR_NULL(ppArgs = ::CommandLineToArgvW(pCommandLine, &numArgs));
  if (numArgs >= 2)
  {
    MJ_ERR_HRESULT(::StringCchCopyW(s_pArg, MJ_COUNTOF(s_pArg), ppArgs[1]));
  }
  MJ_ERR_NONNULL(::LocalFree(ppArgs));

  // Don't bloat the stack
  static mj::FloatMagic floatMagic;

  MJ_ERR_ZERO(::HeapSetInformation(nullptr, HeapEnableTerminationOnCorruption, nullptr, 0));
  mj::ThreadpoolInit();

  static constexpr const auto className = L"Class Name";

  WNDCLASS wc      = {};
  wc.lpfnWndProc   = mj::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = className;
  MJ_ERR_ZERO(::RegisterClassW(&wc));

  // Loads DLLs: uxtheme, combase, msctf, oleaut32
  s_MainWindowHandle =
      ::CreateWindowExW(0,                                                          // Optional window styles.
                        className,                                                  // Window class
                        L"Window Title",                                            // Window text
                        WS_OVERLAPPEDWINDOW | WS_VSCROLL,                           // Window style
                        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, // Size and position
                        nullptr,                                                    // Parent window
                        nullptr,                                                    // Menu
                        HINST_THISCOMPONENT,                                        // Instance handle
                        &floatMagic);                                               // Additional application data
  MJ_ERR_NULL(s_MainWindowHandle);

  // If the window was previously visible, the return value is nonzero.
  // If the window was previously hidden, the return value is zero.
  static_cast<void>(::ShowWindow(s_MainWindowHandle, SW_SHOW));

  // Create accelerator table
  MJ_UNINITIALIZED HACCEL pAcceleratorTable;
  ACCEL table[] = { { FCONTROL | FVIRTKEY, 'O', ID_FILE_OPEN } };
  MJ_ERR_NULL(pAcceleratorTable = ::CreateAcceleratorTableW(table, 1));

  // Run the message loop.
  MSG msg = {};
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    if (::TranslateAcceleratorW(s_MainWindowHandle, pAcceleratorTable, &msg))
    {
      continue;
    }
    static_cast<void>(::TranslateMessage(&msg));
    static_cast<void>(::DispatchMessageW(&msg));
  }

  mj::ThreadpoolDestroy();
}
