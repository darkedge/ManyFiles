#pragma once

namespace mj
{
  class AllocatorBase;
  class Control;

  bool LoadWindowLayout(mj::AllocatorBase* pAllocator);
  void SaveWindowLayout(mj::Control* pRootControl);
} // namespace mj
