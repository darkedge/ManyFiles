#pragma once
#include "mj_allocator.h"

struct IDWriteFactory;
struct ID2D1RenderTarget;
struct IWICImagingFactory;

namespace svc
{
  void Init(mj::AllocatorBase* pAllocator);
  void ProvideGeneralPurposeAllocator(mj::AllocatorBase* pAllocator);
  void ProvideDWriteFactory(IDWriteFactory* pFactory);
  void ProvideD2D1RenderTarget(ID2D1RenderTarget* pContext);
  void ProvideWicFactory(IWICImagingFactory* pContext);
  void ProvideMainWindowHandle(HWND hWnd);
} // namespace svc
