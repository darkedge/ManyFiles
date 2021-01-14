#pragma once
#include "IControl.h"
#include "mj_win32.h"
#include "mj_allocator.h"
#include <d2d1_1.h>
#include <dxgi1_2.h>
#include <d3d11.h>

namespace mj
{
  class MainWindow
  {
  private:
    IControl* pDirectoryNavigationPanel = nullptr;
    ID2D1DCRenderTarget* pRenderTarget  = nullptr;
    bool s_Resize                       = false;

    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    void Init();
    void Resize();
    void Paint();
    void Destroy();

    void ReleaseUnusedObjects();

  public:
    void Run();
    void OnCreateID3D11Device(ID3D11Device* pD3d11Device, IDXGIDevice1* pDxgiDevice);
    void OnCreateID2D1RenderTarget(ID2D1DCRenderTarget* pRenderTarget);
    void OnCreateIDXGISwapChain1(IDXGISwapChain1* pSwapChain);
  };

} // namespace mj
