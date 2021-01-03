#include "ServiceLocator.h"
#include "ServiceProvider.h"
#include "ErrorExit.h"

static IDWriteFactory* pDWriteFactory;
static ID2D1DeviceContext* pD2D1DeviceContext;
static IWICImagingFactory* pWicFactory;
static HWND hWnd;

// TODO: Provide fallback if a service is null

IDWriteFactory* svc::DWriteFactory()
{
  MJ_EXIT_NULL(pDWriteFactory);
  return pDWriteFactory;
}

void svc::ProvideDWriteFactory(IDWriteFactory* pFactory)
{
  pDWriteFactory = pFactory;
}

ID2D1DeviceContext* svc::D2D1DeviceContext()
{
  MJ_EXIT_NULL(pD2D1DeviceContext);
  return pD2D1DeviceContext;
}

void svc::ProvideD2D1DeviceContext(ID2D1DeviceContext* pContext)
{
  pD2D1DeviceContext = pContext;
}

IWICImagingFactory* svc::WicFactory()
{
  MJ_EXIT_NULL(pWicFactory);
  return pWicFactory;
}

void svc::ProvideWicFactory(IWICImagingFactory* pFactory)
{
  pWicFactory = pFactory;
}
