#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"

/// <summary>
/// Exits on failed DirectWrite/Direct2D actions
/// </summary>
/// <param name="hwnd"></param>
/// <param name="pDirect2D"></param>
/// <returns></returns>
void mj::Direct2DInit(HWND hwnd, mj::Direct2D* pDirect2D)
{
  MJ_ERR_NULL(hwnd);
  MJ_ERR_NULL(pDirect2D);

  // DirectWrite factory
  IDWriteFactory* pDWriteFactory = {};
  MJ_ERR_HRESULT(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                     reinterpret_cast<IUnknown**>(&pDWriteFactory)));

  // Direct2D factory
  ID2D1Factory* pDirect2DFactory = {};
  MJ_ERR_HRESULT(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pDirect2DFactory));

  // Get the client area size
  RECT rect = {};
  MJ_ERR_ZERO(GetClientRect(hwnd, &rect));
  const D2D1_SIZE_U d2dSize = D2D1::SizeU(rect.right, rect.bottom);

  // Create render target with provided size
  ID2D1HwndRenderTarget* pRenderTarget = {};
  MJ_ERR_HRESULT(pDirect2DFactory->CreateHwndRenderTarget(
      D2D1::RenderTargetProperties(), D2D1::HwndRenderTargetProperties(hwnd, d2dSize), &pRenderTarget));

  pDirect2D->pDirect2DFactory = pDirect2DFactory;
  pDirect2D->pDWriteFactory   = pDWriteFactory;
  pDirect2D->pRenderTarget    = pRenderTarget;
}
