#pragma once
#include "Control.h"

namespace mj
{
  /// <summary>
  /// This control does not own anything.
  /// Instead, the resizing of its adjacent controls is signalled by the parent layout.
  /// </summary>
  class VerticalResizeControl : public Control
  {
  public:
    static constexpr const auto WIDTH = 8;

    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual void Destroy() override;
  };
} // namespace mj
