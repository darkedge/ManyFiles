#pragma once
#include <Windows.h>

struct IDWriteFactory;
struct ID2D1DeviceContext;
struct IWICImagingFactory;

namespace svc
{
  IDWriteFactory* DWriteFactory();
  ID2D1DeviceContext* D2D1DeviceContext();
  IWICImagingFactory* WicFactory();
  HWND MainWindowHandle();
} // namespace svc
