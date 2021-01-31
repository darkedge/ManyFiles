#pragma once
#include "mj_allocator.h"
#include <Windows.h>

struct ID2D1RenderTarget;

namespace mj
{
  class Control
  {
  public:
    virtual void Init(AllocatorBase* pAllocator)         = 0;
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) = 0;
    virtual void Destroy()                               = 0;

    virtual void OnMouseMove(int16_t x, int16_t y)
    {
      static_cast<void>(x);
      static_cast<void>(y);
    }

    /// <param name="mkMask">Key State Masks for Mouse Messages (MK_*, see WinUser.h)</param>
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
    {
      static_cast<void>(x);
      static_cast<void>(y);
      static_cast<void>(mkMask);
    }

    /// <param name="mkMask">Key State Masks for Mouse Messages (MK_*, see WinUser.h)</param>
    /// <param name="zDelta">Some ratio of WHEEL_DELTA</param>
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta)
    {
      static_cast<void>(x);
      static_cast<void>(y);
      static_cast<void>(mkMask);
      static_cast<void>(zDelta);
    }

    /// <summary>
    /// Context menu intent.
    /// From MSDN:
    /// If the context menu is generated from the keyboard for example, if the user types
    /// SHIFT+F10 then the x- and y-coordinates are -1 and the application
    /// should display the context menu at the location of the current selection
    /// rather than at (xPos, yPos).
    /// </summary>
    virtual void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY)
    {
      static_cast<void>(clientX);
      static_cast<void>(clientY);
      static_cast<void>(screenX);
      static_cast<void>(screenY);
    }

    /// <summary>
    /// Position and dimensions should be adjusted by parent control before calling this function.
    /// </summary>
    virtual void OnSize(){};

    void OnPaint();

    /// <summary>
    /// For use with mouse events.
    /// Translates pPoints to control space if true.
    /// </summary>
    bool TranslateClientPoint(int16_t* pX, int16_t* pY);

    /// <summary>
    /// X position relative to parent control.
    /// Note: Use zero if you want to specify the left edge inside the Paint() method.
    /// </summary>
    int16_t xParent = 0;

    /// <summary>
    /// Y position relative to parent control.
    /// Note: Use zero if you want to specify the top edge inside the Paint() method.
    /// </summary>
    int16_t yParent = 0;

    int16_t width  = 0;
    int16_t height = 0;
  };
} // namespace mj
