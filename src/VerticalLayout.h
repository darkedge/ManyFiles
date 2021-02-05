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

    /// <summary>
    /// Does not own the Control.
    /// </summary>
    void Add(Control* pControl);

  protected:
    virtual void MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t dx,
                                   int16_t dy) override;
  };
} // namespace mj
