#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"

struct CreateHwndRenderTargetContext
{
  // In
  HWND pHwnd;
  ID2D1Factory* pDirect2DFactory;

  // Out
  ID2D1HwndRenderTarget* pRenderTarget;
};

static void CreateHwndRenderTarget(TP_CALLBACK_INSTANCE* pInstance, void* pContext, TP_WORK* pWork)
{
  static_cast<void>(pInstance);
  static_cast<void>(pWork);

  CreateHwndRenderTargetContext* pTaskContext = static_cast<CreateHwndRenderTargetContext*>(pContext);

  // Get the client area size
  RECT rect = {};
  MJ_ERR_ZERO(::GetClientRect(pTaskContext->pHwnd, &rect));
  const D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Create render target with provided size
  MJ_ERR_HRESULT(pTaskContext->pDirect2DFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(pTaskContext->pHwnd, d2dSize),
      &pTaskContext->pRenderTarget));
}

/// <summary>
/// Exits on failed DirectWrite/Direct2D actions
/// </summary>
/// <param name="hwnd"></param>
/// <param name="pDirect2D"></param>
/// <returns></returns>
void mj::Direct2DInit(HWND hwnd, mj::Direct2D* pDirect2D)
{
  MJ_EXIT_NULL(hwnd);
  MJ_EXIT_NULL(pDirect2D);

  mj::Task task                           = mj::ThreadpoolTaskAlloc(CreateHwndRenderTarget);
  CreateHwndRenderTargetContext* pContext = task.Context<CreateHwndRenderTargetContext>();
  pContext->pHwnd                         = hwnd;

  // DirectWrite factory
  MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(&pDirect2D->pDWriteFactory)));

  // Direct2D factory
  MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pContext->pDirect2DFactory));

  // Loads DLLs: dxgi, ResourcePolicyClient, ResourcePolicyClient, kernel.appcore, d3d11, DriverStore\FileRepository\nv_dispi.inf_amd64_95bdb3a23d6478de\nvldumdx, version, crypt32, cryptnet, cryptbase, wintrust, msasn1, imagehlp, cryptsp, rsaenh, bcrypt, DriverStore\FileRepository\nv_dispi.inf_amd64_95bdb3a23d6478de\nvwgf2umx, winmm, DriverStore\FileRepository\nv_dispi.inf_amd64_95bdb3a23d6478de\NvCamera\NvCameraWhitelisting64, shell32, shlwapi, windows.storage, wldp
  // Unloads DLLs: shlwapi, wldp, windows.storage, shell32, DriverStore\FileRepository\nv_dispi.inf_amd64_95bdb3a23d6478de\NvCamera\NvCameraWhitelisting64
  // Loads DLLs: shell32, windows.storage, wldp, SHCore, shlwapi, DXCore, cfgmgr32
  task.Submit();
  task.Wait();

  pDirect2D->pRenderTarget = pContext->pRenderTarget;
  pDirect2D->pDirect2DFactory = pContext->pDirect2DFactory;

  mj::ThreadpoolTaskFree(task);
}
