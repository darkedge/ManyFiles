#pragma once
#include "LinearLayout.h"
#include "mj_common.h"

namespace mj
{
  class HorizontalLayout : public LinearLayout
  {
  public:
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual void OnSize() override;
  };
} // namespace mj
