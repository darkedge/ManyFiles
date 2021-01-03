#pragma once
#include "IControl.h"
#include "mj_win32.h"
#include "mj_allocator.h"
#include <d2d1_1.h>
#include <dxgi1_2.h>

namespace mj
{
  class MainWindow
  {
  private:
    IControl* pDirectoryNavigationPanel = nullptr;
    ID2D1DeviceContext* pDeviceContext  = nullptr;
    IDXGISwapChain1* pSwapChain         = nullptr;
    bool s_Resize                       = false;
    VirtualAllocator allocator;

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Init(HWND hWnd);
    void Resize();
    void Paint();
    void Destroy();

  public:
    void Run();
  };

} // namespace mj
