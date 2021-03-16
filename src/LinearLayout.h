#pragma once
#include "Control.h"
#include "mj_common.h"

namespace mj
{
  class LinearLayout : public Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Destroy() override;
    
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask) override;
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta) override;
    virtual void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY) override;
    virtual void OnMouseMove(MouseMoveEvent* pMouseMoveEvent) override;
    [[nodiscard]] virtual bool OnLeftButtonDown(int16_t x, int16_t y) override;
    [[nodiscard]] virtual bool OnLeftButtonUp(int16_t x, int16_t y) override;

  protected:
    static constexpr const int16_t MIN_PANEL_SIZE = 39;
    
    virtual void SaveToStringInternal(StringBuilder sb, uint16_t offset) override;

    AllocatorBase* pAllocator = nullptr;
    ArrayList<Control*> controls;

    virtual void MoveResizeControl(Control* pFirst, Control* pResizeControl, Control* pSecond, int16_t* pDx,
                                   int16_t* pDy) = 0;

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
