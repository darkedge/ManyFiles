#pragma once
#include "IControl.h"
#include "mj_common.h"
#include "StringBuilder.h"
#include <d2d1_1.h>

namespace mj
{
  struct EEntryType
  {
    enum Enum
    {
      File,
      Directory,
    };
  };
  struct Entry
  {
    EEntryType type;
    IDWriteTextLayout* pTextLayout;
    ID2D1Bitmap1* pIcon;
  };

  class DirectoryNavigationPanel : public IControl
  {
  private:
    ID2D1SolidColorBrush* pBlackBrush;
    IDWriteTextFormat* pTextFormat;
    ID2D1Bitmap1* pFolderIcon;

    AllocatorBase* pAllocator;
    ArrayList<Entry> entries;
    Allocation searchBuffer;
    Allocation resultsBuffer;

    ID2D1Bitmap1* ConvertIcon(HICON hIcon);
    void LoadIcons();
    void Navigate(const String& string);

  public:
    void Init(AllocatorBase* pAllocator) override;
    virtual void Paint() override;
    void Destroy() override;
  };
} // namespace mj
