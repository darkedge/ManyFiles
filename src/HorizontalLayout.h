#pragma once
#include "Control.h"
#include "mj_common.h"

namespace mj
{
  class HorizontalLayout : public Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void OnPaint() override;
    virtual void Destroy() override;
    virtual void OnSize() override;
    
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask) override;
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta) override;
    virtual void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY) override;
    virtual void OnMouseMove(int16_t x, int16_t y) override;

    /// <summary>
    /// Does not own the Control.
    /// </summary>
    void Add(Control* pControl);

  private:
    bool resizable = false;
    ArrayList<Control*> controls;
  };
} // namespace mj
