#pragma once
#include "LinearLayout.h"
#include "mj_common.h"

namespace mj
{
  class VerticalLayout : public LinearLayout
  {
  public:
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual void OnSize() override;
    virtual void Destroy() override;

    /// <summary>
    /// Does not own the Control.
    /// </summary>
    void Add(Control* pControl);
  };
} // namespace mj
