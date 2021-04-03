#pragma once
#include "LinearLayout.h"
#include "mj_common.h"

namespace mj
{
  class HorizontalLayout : public LinearLayout
  {
  public:
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual const wchar_t* GetType() override
    {
      return MJ_NAMEOF(HorizontalLayout);
    }
    virtual void OnSize() override;

    /// <summary>
    /// Does not own the Control.
    /// </summary>
    void Add(Control* pControl);

  protected:
    virtual void MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t* pDx,
                                   int16_t* pDy) override;
  };

  /// <summary>
  /// This control does not own anything.
  /// Instead, the resizing of its adjacent controls is done by the parent layout.
  /// </summary>
  class HorizontalResizeControl : public Control
  {
  public:
    static constexpr const auto WIDTH = 8;

    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual const wchar_t* GetType() override
    {
      return MJ_NAMEOF(HorizontalResizeControl);
    }
    [[nodiscard]] virtual bool OnLeftButtonDown(int16_t x, int16_t y) override;

    virtual void OnMouseMove(MouseMoveEvent* pMouseMoveEvent) override
    {
      pMouseMoveEvent->cursor = res::win32::ECursor::Horizontal;
    }
  };
} // namespace mj
