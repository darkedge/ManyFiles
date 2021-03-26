#pragma once
#include "ResourcesWin32.h"

struct IDCompositionDesktopDevice;
struct IDCompositionVirtualSurface;
struct IDCompositionTarget;
struct ID2D1RenderTarget;

namespace mj
{
  class Control;

  class MainWindow
  {
  private:
    static constexpr const size_t WIDTH     = 3;
    static constexpr const size_t HEIGHT    = 2;
    Control* pRootControl                   = nullptr;
    IDCompositionDesktopDevice* dcompDevice = nullptr;
    IDCompositionVirtualSurface* pSurface   = nullptr;
    IDCompositionTarget* pTarget            = nullptr;
    LONG captionHeight                      = 0;

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Resize();
    void OnPaint();
    void Destroy();

  public:
    void Run();
    void OnCreateID2D1RenderTarget(IDCompositionDesktopDevice* dcompDevice, ID2D1RenderTarget* pRenderTarget);
    LONG GetCaptionHeight()
    {
      return this->captionHeight;
    }
    void SetCaptionHeight(LONG captionHeight)
    {
      this->captionHeight = captionHeight;
    }
  };

} // namespace mj
