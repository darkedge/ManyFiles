#pragma once
#include "mj_allocator.h"

struct IDWriteFactory;
struct ID2D1DeviceContext;
struct IWICImagingFactory;

namespace svc
{
  void Init(mj::AllocatorBase* pAllocator);
  void ProvideDWriteFactory(IDWriteFactory* pFactory);
  void ProvideD2D1DeviceContext(ID2D1DeviceContext* pContext);
  void ProvideWicFactory(IWICImagingFactory* pContext);
  void ProvideMainWindowHandle(HWND hWnd);
} // namespace svc
