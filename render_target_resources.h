#pragma once
struct ID2D1SolidColorBrush;

struct RenderTargetResources
{
  ID2D1SolidColorBrush* pTextBrush;
  ID2D1SolidColorBrush* pTextEditBackgroundBrush;
  ID2D1SolidColorBrush* pScrollBarBrush;
  ID2D1SolidColorBrush* pScrollBarBackgroundBrush;
};
