#pragma once
#include "mj_allocator.h"
#include <Windows.h>

struct IDWriteFactory;
struct ID2D1RenderTarget;
struct IWICImagingFactory;

namespace svc
{
  // IDWriteFactory
  IDWriteFactory* DWriteFactory();
  class IDWriteFactoryObserver
  {
  public:
    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) = 0;
  };
  void AddIDWriteFactoryObserver(IDWriteFactoryObserver* pObserver);
  void RemoveIDWriteFactoryObserver(IDWriteFactoryObserver* pObserver);

  // ID2D1RenderTarget
  ID2D1RenderTarget* D2D1RenderTarget();
  class ID2D1RenderTargetObserver
  {
  public:
    virtual void OnID2D1RenderTargetAvailable(ID2D1RenderTarget* pContext) = 0;
  };
  void AddID2D1RenderTargetObserver(ID2D1RenderTargetObserver* pObserver);
  void RemoveID2D1RenderTargetObserver(ID2D1RenderTargetObserver* pObserver);

  // IWICImagingFactory
  IWICImagingFactory* WicFactory();
  class IWICFactoryObserver
  {
  public:
    virtual void OnWICFactoryAvailable(IWICImagingFactory* pFactory) = 0;
  };
  void AddWicFactoryObserver(IWICFactoryObserver* pObserver);
  void RemoveWicFactoryObserver(IWICFactoryObserver* pObserver);

  HWND MainWindowHandle();
  mj::AllocatorBase* GeneralPurposeAllocator();
} // namespace svc
