#include "ServiceLocator.h"
#include "ServiceProvider.h"
#include "ErrorExit.h"
#include "mj_common.h"

static IDWriteFactory* pDWriteFactory;
static ID2D1DeviceContext* pD2D1DeviceContext;
static IWICImagingFactory* pWicFactory;
static HWND hWnd;
// TODO: These should be sets, not arrays
static mj::ArrayList<svc::IWICFactoryObserver*> s_WicFactoryObservers;
static mj::ArrayList<svc::ID2D1DeviceContextObserver*> s_ID2D1DeviceContextObservers;
static mj::ArrayList<svc::IDWriteFactoryObserver*> s_IDWriteFactoryObservers;

// TODO: Provide fallback if a service is null

void svc::Init(mj::AllocatorBase* pAllocator)
{
  s_WicFactoryObservers.Init(pAllocator);
  s_ID2D1DeviceContextObservers.Init(pAllocator);
  s_IDWriteFactoryObservers.Init(pAllocator);
}

IDWriteFactory* svc::DWriteFactory()
{
  MJ_EXIT_NULL(pDWriteFactory);
  return pDWriteFactory;
}

void svc::ProvideDWriteFactory(IDWriteFactory* pFactory)
{
  pDWriteFactory = pFactory;
  for (auto pObserver : s_IDWriteFactoryObservers)
  {
    pObserver->OnIDWriteFactoryAvailable(pDWriteFactory);
  }
}

void svc::AddIDWriteFactoryObserver(IDWriteFactoryObserver* pObserver)
{
  s_IDWriteFactoryObservers.Add(pObserver);
  // Immediately notify the observer if the service is provided
  if (pDWriteFactory)
  {
    pObserver->OnIDWriteFactoryAvailable(pDWriteFactory);
  }
}

void svc::RemoveIDWriteFactoryObserver(IDWriteFactoryObserver* pObserver)
{
  s_IDWriteFactoryObservers.RemoveAll(pObserver);
}

ID2D1DeviceContext* svc::D2D1DeviceContext()
{
  MJ_EXIT_NULL(pD2D1DeviceContext);
  return pD2D1DeviceContext;
}

void svc::ProvideD2D1DeviceContext(ID2D1DeviceContext* pContext)
{
  pD2D1DeviceContext = pContext;
  for (auto pObserver : s_ID2D1DeviceContextObservers)
  {
    pObserver->OnID2D1DeviceContextAvailable(pD2D1DeviceContext);
  }
}

void svc::AddID2D1DeviceContextObserver(ID2D1DeviceContextObserver* pObserver)
{
  s_ID2D1DeviceContextObservers.Add(pObserver);
  // Immediately notify the observer if the service is provided
  if (pD2D1DeviceContext)
  {
    pObserver->OnID2D1DeviceContextAvailable(pD2D1DeviceContext);
  }
}

void svc::RemoveID2D1DeviceContextObserver(ID2D1DeviceContextObserver* pObserver)
{
  s_ID2D1DeviceContextObservers.RemoveAll(pObserver);
}

IWICImagingFactory* svc::WicFactory()
{
  MJ_EXIT_NULL(pWicFactory);
  return pWicFactory;
}

void svc::ProvideWicFactory(IWICImagingFactory* pFactory)
{
  pWicFactory = pFactory;
  for (auto pObserver : s_WicFactoryObservers)
  {
    pObserver->OnWICFactoryAvailable(pWicFactory);
  }
}

void svc::AddWicFactoryObserver(IWICFactoryObserver* pObserver)
{
  s_WicFactoryObservers.Add(pObserver);
  // Immediately notify the observer if the service is provided
  if (pWicFactory)
  {
    pObserver->OnWICFactoryAvailable(pWicFactory);
  }
}

void svc::RemoveWicFactoryObserver(IWICFactoryObserver* pObserver)
{
  s_WicFactoryObservers.RemoveAll(pObserver);
}
