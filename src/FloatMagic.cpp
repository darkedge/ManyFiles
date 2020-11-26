#include "FloatMagic.h"
#include "..\3rdparty\tracy\Tracy.hpp"
#include "vld.h"
#include "mj_win32.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Direct2D.h"
#include "Threadpool.h"
#include "ConvertToHex.h"

#include <ShlObj.h>

static constexpr UINT ID_FILE_OPEN = 40001;

namespace mj
{
  static void CreateMenu(HWND pHwnd)
  {
    HMENU hMenu = ::CreateMenu();
    MJ_ERR_NULL(hMenu);

    MJ_UNINITIALIZED HMENU hSubMenu;

    MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
    ::AppendMenuW(hSubMenu, MF_STRING, ID_FILE_OPEN, L"&Open...\tCtrl+O");
    MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&File"));

    MJ_ERR_NULL(hSubMenu = ::CreatePopupMenu());
    MJ_ERR_ZERO(::AppendMenuW(hMenu, MF_STRING | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu), L"&Reports"));

    // Loads DLLs: TextShaping
    MJ_ERR_ZERO(::SetMenu(pHwnd, hMenu));
  }

  void OpenFileDialog(HWND pHwnd)
  {
    ZoneScoped;
    mj::ComPtr<::IFileOpenDialog> pFileOpen;

    // Create the FileOpenDialog object.
    // TODO MJ: Create once?
    HRESULT hr = ::CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog,
                                    reinterpret_cast<LPVOID*>(pFileOpen.GetAddressOf()));

    if (SUCCEEDED(hr))
    {
      hr = pFileOpen->Show(pHwnd);
    }

    // Get the file name from the dialog box.
    mj::ComPtr<::IShellItem> pItem;
    if (SUCCEEDED(hr))
    {
      hr = pFileOpen->GetResult(pItem.GetAddressOf());
    }

    PWSTR pszFilePath = nullptr;
    if (SUCCEEDED(hr))
    {
      hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
    }

    if (SUCCEEDED(hr))
    {
      HANDLE hFile = CreateFileW(pszFilePath,           // file to open
                                 GENERIC_READ,          // open for reading
                                 FILE_SHARE_READ,       // share for reading
                                 nullptr,               // default security
                                 OPEN_EXISTING,         // existing file only
                                 FILE_ATTRIBUTE_NORMAL, // normal file
                                 nullptr);              // no attr. template

      if (hFile != INVALID_HANDLE_VALUE)
      {
        MJ_UNINITIALIZED DWORD dwBytesRead;
        constexpr DWORD BUFFERSIZE = 1024; // TODO MJ: Fixed buffer size
        char ReadBuffer[BUFFERSIZE];

        MJ_ERR_ZERO(::ReadFile(hFile, ReadBuffer, BUFFERSIZE - 1, &dwBytesRead, nullptr));

        ReadBuffer[dwBytesRead] = '\0';

        MJ_ERR_ZERO(CloseHandle(hFile));
      }

      ::CoTaskMemFree(pszFilePath);
    }
  }

  /// <summary>
  /// Processes WM_COMMAND messages from the main WindowProc further.
  /// </summary>
  /// <param name="commandId"></param>
  void WindowProcCommand(FloatMagic* pFloatMagic, UINT commandId)
  {
    ZoneScoped;

    switch (commandId)
    {
    case ID_FILE_OPEN:
      mj::OpenFileDialog(pFloatMagic->pHwnd);
      break;
    }
  }

  LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
  {
    mj::FloatMagic* pMainWindow = reinterpret_cast<mj::FloatMagic*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (message)
    {
    case WM_NCCREATE:
    {
      // Copy the lpParam from CreateWindowEx to this window's user data
      CREATESTRUCT* pcs  = reinterpret_cast<CREATESTRUCT*>(lParam);
      pMainWindow        = reinterpret_cast<mj::FloatMagic*>(pcs->lpCreateParams);
      pMainWindow->pHwnd = hwnd;
      MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));

      mj::CreateMenu(hwnd);

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
    case WM_COMMAND: // Fall-through
      mj::WindowProcCommand(pMainWindow, static_cast<UINT>(LOWORD(wParam)));
      break;
    default:
      break;
    }

    return ::DefWindowProcW(hwnd, message, wParam, lParam);
  }
} // namespace mj

void mj::FloatMagicMain()
{
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

  // Create accelerator table
  MJ_UNINITIALIZED HACCEL pAcceleratorTable;
  ACCEL table[] = { { FCONTROL | FVIRTKEY, 'O', ID_FILE_OPEN } };
  MJ_ERR_NULL(pAcceleratorTable = ::CreateAcceleratorTableW(table, 1));

  // Run the message loop.
  MSG msg = {};
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    if (::TranslateAcceleratorW(pHwnd, pAcceleratorTable, &msg))
    {
      continue;
    }
    static_cast<void>(::TranslateMessage(&msg));
    static_cast<void>(::DispatchMessageW(&msg));
  }

  mj::ThreadpoolDestroy();
}
