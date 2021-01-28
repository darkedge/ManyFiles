#include "ServiceLocator.h"
#include "ServiceProvider.h"
#include "ErrorExit.h"
#include "mj_common.h"
#include "../3rdparty/tracy/Tracy.hpp"

static IDWriteFactory* pDWriteFactory;
static ID2D1RenderTarget* pD2D1RenderTarget;
static IWICImagingFactory* pWicFactory;
static HWND hWnd;
static mj::AllocatorBase* s_pGeneralPurposeAllocator;

// TODO: These should be sets, not arrays
static mj::ArrayList<svc::IWICFactoryObserver*> s_WicFactoryObservers;
static mj::ArrayList<svc::ID2D1RenderTargetObserver*> s_ID2D1RenderTargetObservers;
static mj::ArrayList<svc::IDWriteFactoryObserver*> s_IDWriteFactoryObservers;

// TODO: Provide fallback if a service is null

void svc::Init(mj::AllocatorBase* pAllocator)
{
  ZoneScoped;
  s_WicFactoryObservers.Init(pAllocator);
  s_ID2D1RenderTargetObservers.Init(pAllocator);
  s_IDWriteFactoryObservers.Init(pAllocator);
}

void svc::Destroy()
{
  ZoneScoped;
  s_WicFactoryObservers.Destroy();
  s_ID2D1RenderTargetObservers.Destroy();
  s_IDWriteFactoryObservers.Destroy();
}

mj::AllocatorBase* svc::GeneralPurposeAllocator()
{
  MJ_EXIT_NULL(s_pGeneralPurposeAllocator);
  return s_pGeneralPurposeAllocator;
}

void svc::ProvideGeneralPurposeAllocator(mj::AllocatorBase* pAllocator)
{
  s_pGeneralPurposeAllocator = pAllocator;
}

IDWriteFactory* svc::DWriteFactory()
{
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

ID2D1RenderTarget* svc::D2D1RenderTarget()
{
  return pD2D1RenderTarget;
}

void svc::ProvideD2D1RenderTarget(ID2D1RenderTarget* pRenderTarget)
{
  pD2D1RenderTarget = pRenderTarget;
  for (auto pObserver : s_ID2D1RenderTargetObservers)
  {
    pObserver->OnID2D1RenderTargetAvailable(pD2D1RenderTarget);
  }
}

void svc::AddID2D1RenderTargetObserver(ID2D1RenderTargetObserver* pObserver)
{
  s_ID2D1RenderTargetObservers.Add(pObserver);
  // Immediately notify the observer if the service is provided
  if (pD2D1RenderTarget)
  {
    pObserver->OnID2D1RenderTargetAvailable(pD2D1RenderTarget);
  }
}

void svc::RemoveID2D1RenderTargetObserver(ID2D1RenderTargetObserver* pObserver)
{
  s_ID2D1RenderTargetObservers.RemoveAll(pObserver);
}

IWICImagingFactory* svc::WicFactory()
{
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

HWND svc::MainWindowHandle()
{
  MJ_EXIT_NULL(hWnd);
  return hWnd;
}

void svc::ProvideMainWindowHandle(HWND handle)
{
  hWnd = handle;
}
