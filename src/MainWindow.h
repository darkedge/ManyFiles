#pragma once
#include "HorizontalLayout.h"
#include "VerticalLayout.h"
#include "mj_win32.h"
#include "mj_allocator.h"
#include <d2d1_1.h>
#include <dxgi1_3.h>
#include <d3d11.h>
#include <dcomp.h>

namespace mj
{
  class MainWindow
  {
  private:
    static constexpr const size_t WIDTH  = 3;
    static constexpr const size_t HEIGHT = 2;
    Control* controls[WIDTH * HEIGHT];
    HorizontalLayout* pHorizontalLayouts[HEIGHT];
    VerticalLayout* pRootControl            = nullptr;
    IDCompositionDesktopDevice* dcompDevice = nullptr;
    IDCompositionVirtualSurface* pSurface   = nullptr;

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Resize();
    void OnPaint();
    void Destroy();

  public:
    void Run();
    void OnCreateID2D1RenderTarget(IDCompositionDesktopDevice* dcompDevice, ID2D1RenderTarget* pRenderTarget);
  };

} // namespace mj
