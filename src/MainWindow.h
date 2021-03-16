#pragma once
#include "ResourcesWin32.h"

struct IDCompositionDesktopDevice;
struct IDCompositionVirtualSurface;
struct IDCompositionTarget;
struct ID2D1RenderTarget;

namespace mj
{
  class Control;
  class HorizontalLayout;
  class VerticalLayout;

  class MainWindow
  {
  private:
    static constexpr const size_t WIDTH          = 3;
    static constexpr const size_t HEIGHT         = 2;
    Control* controls[WIDTH * HEIGHT]            = {};
    HorizontalLayout* pHorizontalLayouts[HEIGHT] = {};
    VerticalLayout* pRootControl                 = nullptr;
    IDCompositionDesktopDevice* dcompDevice      = nullptr;
    IDCompositionVirtualSurface* pSurface        = nullptr;
    IDCompositionTarget* pTarget                 = nullptr;

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Resize();
    void OnPaint();
    void Destroy();

    void SaveLayoutToFile();

  public:
    void Run();
    void OnCreateID2D1RenderTarget(IDCompositionDesktopDevice* dcompDevice, ID2D1RenderTarget* pRenderTarget);
  };

} // namespace mj
