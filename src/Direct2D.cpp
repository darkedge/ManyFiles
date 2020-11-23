#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"

IDWriteFactory* pDWriteFactory;
ID2D1Factory* pDirect2DFactory;
ID2D1HwndRenderTarget* pRenderTarget;

struct CreateHwndRenderTargetContext
{
  // In
  HWND pHwnd;

  // Out
  IDWriteFactory* pDWriteFactory;
  ID2D1Factory* pDirect2DFactory;
  ID2D1HwndRenderTarget* pRenderTarget;
};

static void CreateHwndRenderTargetFinish(mj::Task* pTask)
{
  CreateHwndRenderTargetContext* pContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTask->pContext);

  pDWriteFactory   = pContext->pDWriteFactory;
  pDirect2DFactory = pContext->pDirect2DFactory;
  pRenderTarget    = pContext->pRenderTarget;
}

static void CreateHwndRenderTarget(TP_CALLBACK_INSTANCE* pInstance, void* pContext, TP_WORK* pWork)
{
  static_cast<void>(pInstance);
  static_cast<void>(pWork);

  mj::Task* pTask = static_cast<mj::Task*>(pContext);

  CreateHwndRenderTargetContext* pTaskContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTask->pContext);

  // DirectWrite factory
  MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(&pTaskContext->pDWriteFactory)));

  // Direct2D factory
  MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pTaskContext->pDirect2DFactory));

  // Get the client area size
  RECT rect = {};
  MJ_ERR_ZERO(::GetClientRect(pTaskContext->pHwnd, &rect));
  const D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Note: This call is slow as it loads a LOT of DLLs.
  MJ_ERR_HRESULT(pTaskContext->pDirect2DFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(pTaskContext->pHwnd, d2dSize),
      &pTaskContext->pRenderTarget));

  MJ_ERR_ZERO(::PostMessageW(pTaskContext->pHwnd, MJ_TASKEND, reinterpret_cast<WPARAM>(pTask),
                             reinterpret_cast<LPARAM>(CreateHwndRenderTargetFinish)));
}

/// <summary>
/// Exits on failed DirectWrite/Direct2D actions
/// </summary>
/// <param name="hwnd"></param>
/// <param name="pDirect2D"></param>
/// <returns></returns>
void mj::Direct2DInit(HWND hwnd)
{
  MJ_EXIT_NULL(hwnd);

  mj::Task* pTask = mj::ThreadpoolTaskAlloc(CreateHwndRenderTarget);

  CreateHwndRenderTargetContext* pContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTask->pContext);
  pContext->pHwnd                         = hwnd;

  pTask->Submit();
}
