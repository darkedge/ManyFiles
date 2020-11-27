#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"
#include "minicrt.h"

static IDWriteFactory* pDWriteFactory;
static ID2D1Factory* pDirect2DFactory;
static ID2D1HwndRenderTarget* pRenderTarget;

struct CreateHwndRenderTargetContext
{
  // In
  HWND hWnd;

  // Out
  IDWriteFactory* pDWriteFactory;
  ID2D1Factory* pDirect2DFactory;
  ID2D1HwndRenderTarget* pRenderTarget;
};

static void CreateHwndRenderTargetFinish(const mj::TaskContext* pTaskContext)
{
  const CreateHwndRenderTargetContext* pContext = reinterpret_cast<const CreateHwndRenderTargetContext*>(pTaskContext);

  pDWriteFactory   = pContext->pDWriteFactory;
  pDirect2DFactory = pContext->pDirect2DFactory;
  pRenderTarget    = pContext->pRenderTarget;

  pRenderTarget->BeginDraw();
  pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::CornflowerBlue));
  pRenderTarget->EndDraw();
}

static void InitDirect2DAsync(mj::TaskContext* pTaskContext)
{
  CreateHwndRenderTargetContext* pContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTaskContext);

  // DirectWrite factory
  MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(&pContext->pDWriteFactory)));

  // Direct2D factory
  MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pContext->pDirect2DFactory));

  // Get the client area size
  RECT rect = {};
  MJ_ERR_ZERO(::GetClientRect(pContext->hWnd, &rect));
  const D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Note: This call is slow as it loads a LOT of DLLs.
  MJ_ERR_HRESULT(pContext->pDirect2DFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(pContext->hWnd, d2dSize),
      &pContext->pRenderTarget));
}

void mj::Direct2DInit(HWND hwnd)
{
  MJ_EXIT_NULL(hwnd);

  mj::Task* pTask = mj::ThreadpoolTaskAlloc(InitDirect2DAsync, CreateHwndRenderTargetFinish);

  CreateHwndRenderTargetContext* pContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTask->pContext);
  pContext->hWnd                          = hwnd;

  pTask->Submit();
}
