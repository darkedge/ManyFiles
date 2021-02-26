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

    /// <summary>
    /// Does not own the Control.
    /// </summary>
    void Add(Control* pControl);

  protected:
    virtual void MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t* pDx,
                                   int16_t* pDy) override;
  };
} // namespace mj
