#pragma once
#include "Control.h"
#include "mj_common.h"

namespace mj
{
  class TabLayout : public Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Destroy() override;
    
    virtual void OnMouseMove(MouseMoveEvent* pMouseMoveEvent) override;
    [[nodiscard]] virtual bool OnLeftButtonDown(int16_t x, int16_t y) override;
    [[nodiscard]] virtual bool OnLeftButtonUp(int16_t x, int16_t y) override;

  protected:
    static constexpr const int16_t MIN_PANEL_SIZE = 39;
    
    virtual void SaveToStringInternal(StringBuilder sb, uint16_t offset) override;

    AllocatorBase* pAllocator = nullptr;
    ArrayList<Control*> controls;

  private:
    /// <summary>
    /// Value starts at one and increments in uneven numbers.
    /// Zero means no resize control selected.
    /// </summary>
    size_t resizeControlIndex = 0;
    MJ_UNINITIALIZED int16_t dragStartX;
    MJ_UNINITIALIZED int16_t dragStartY;
  };
} // namespace mj
