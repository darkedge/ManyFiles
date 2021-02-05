#pragma once
#include "Control.h"

namespace mj
{
  /// <summary>
  /// This control does not own anything.
  /// Instead, the resizing of its adjacent controls is done by the parent layout.
  /// </summary>
  class HorizontalResizeControl : public Control
  {
  public:
    static constexpr const auto HEIGHT = 8;

    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual void Destroy() override;
    [[nodiscard]] virtual bool OnLeftButtonDown(int16_t x, int16_t y) override;
  };
} // namespace mj
