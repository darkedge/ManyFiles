#pragma once
#include <Windows.h>

struct IDWriteFactory;
struct ID2D1DeviceContext;
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
  
  // ID2D1DeviceContext
  ID2D1DeviceContext* D2D1DeviceContext();
  class ID2D1DeviceContextObserver
  {
  public:
    virtual void OnID2D1DeviceContextAvailable(ID2D1DeviceContext* pContext) = 0;
  };
  void AddID2D1DeviceContextObserver(ID2D1DeviceContextObserver* pObserver);
  void RemoveID2D1DeviceContextObserver(ID2D1DeviceContextObserver* pObserver);

  // IWICImagingFactory
  IWICImagingFactory* WicFactory();
  class IWICFactoryObserver
  {
  public:
    virtual void OnWICFactoryAvailable(IWICImagingFactory* pFactory) = 0;
  };
  void AddWicFactoryObserver(IWICFactoryObserver* pObserver);
  void RemoveWicFactoryObserver(IWICFactoryObserver* pObserver);
} // namespace svc
