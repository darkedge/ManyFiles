#pragma once
#include "mj_win32.h"

struct ID2D1SolidColorBrush;

struct RenderTargetResources
{
  mj::ComPtr<ID2D1SolidColorBrush> pTextBrush;
  mj::ComPtr<ID2D1SolidColorBrush> pTextEditBackgroundBrush;
  mj::ComPtr<ID2D1SolidColorBrush> pScrollBarBrush;
  mj::ComPtr<ID2D1SolidColorBrush> pCaretBrush;
  mj::ComPtr<ID2D1SolidColorBrush> pScrollBarBackgroundBrush;
  mj::ComPtr<ID2D1SolidColorBrush> pScrollBarHighlightBrush;
};
