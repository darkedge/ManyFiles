#include "Direct2D.h"
#include "mj_common.h"
#include "ErrorExit.h"
#include "Threadpool.h"
#include "File.h"
#include "FloatMagic.h"
#include "HexEdit.h"
#include <strsafe.h>
#include "minicrt.h"

static IDWriteFactory* pDWriteFactory;
static ID2D1Factory* pDirect2DFactory;
static ID2D1HwndRenderTarget* pRenderTarget;

static IDWriteTextLayout* s_pTextLayout;
static IDWriteTextFormat* s_pTextFormat;
static ID2D1SolidColorBrush* s_pBrush;

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

  mj::RenderHexEditBuffer();
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

  // Init-once stuff
  MJ_ERR_HRESULT(pContext->pDWriteFactory->CreateTextFormat(L"Consolas",                // Font name
                                                            nullptr,                    // Font collection
                                                            DWRITE_FONT_WEIGHT_NORMAL,  // Font weight
                                                            DWRITE_FONT_STYLE_NORMAL,   // Font style
                                                            DWRITE_FONT_STRETCH_NORMAL, // Font stretch
                                                            16.0f,                      // Font size
                                                            L"",                        // Locale name
                                                            &s_pTextFormat));

  MJ_ERR_HRESULT(pContext->pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &s_pBrush));
}

void mj::Direct2DInit(HWND hwnd)
{
  MJ_EXIT_NULL(hwnd);

  mj::Task* pTask = mj::ThreadpoolTaskAlloc(InitDirect2DAsync, CreateHwndRenderTargetFinish);

  CreateHwndRenderTargetContext* pContext = reinterpret_cast<CreateHwndRenderTargetContext*>(pTask->pContext);
  pContext->hWnd                          = hwnd;

  pTask->Submit();
}

/// <summary>
/// Main thread
/// </summary>
void mj::RenderHexEditBuffer()
{
  const mj::WideString string = mj::HexEditGetBuffer();
  if (string.pString && pDWriteFactory && pRenderTarget)
  {
    if (s_pTextLayout)
    {
      s_pTextLayout->Release();
    }
    MJ_ERR_HRESULT(pDWriteFactory->CreateTextLayout(string.pString, string.length, s_pTextFormat, 1000.0f, 1000.0f,
                                                    &s_pTextLayout));
  }
}

UINT mj::GetRenderedTextHeight()
{
  UINT height = 0;

  if (s_pTextLayout)
  {
    MJ_UNINITIALIZED DWRITE_TEXT_METRICS metrics;
    MJ_ERR_HRESULT(s_pTextLayout->GetMetrics(&metrics));
    height = static_cast<UINT>(metrics.height);
  }

  return height;
}

void mj::Direct2DDraw(int y)
{
  if (pRenderTarget && s_pTextLayout)
  {
    pRenderTarget->BeginDraw();

    // Note MJ: Clear the screen once to a different color to get a sense of the loading time
    pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

    // This can be very slow, once the number of characters goes into the millions.
    // Even in release mode!
    pRenderTarget->DrawTextLayout(D2D1::Point2F(0.0f, static_cast<FLOAT>(-y)), s_pTextLayout, s_pBrush,
                                  D2D1_DRAW_TEXT_OPTIONS_CLIP);

    pRenderTarget->EndDraw();
  }
}

/// <summary>
/// Resizes the render target.
/// </summary>
void mj::Direct2DOnSize(WORD width, WORD height)
{
  if (pRenderTarget)
  {
    MJ_UNINITIALIZED D2D1_SIZE_U size;
    size.width  = width;
    size.height = height;
    pRenderTarget->Resize(&size);
  }
}
