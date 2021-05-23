#pragma once
#include "mj_allocator.h"

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
