#pragma once
#include "IControl.h"
#include "mj_common.h"
#include "mj_string.h"
#include "ServiceLocator.h"
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
    EEntryType::Enum type;
    IDWriteTextLayout* pTextLayout;
    ID2D1Bitmap* pIcon;
  };

  struct FolderContentItem
  {
    EEntryType::Enum type;
    String pName;
  };

  class DirectoryNavigationPanel : public IControl,
                                   public svc::ID2D1RenderTargetObserver,
                                   public svc::IDWriteFactoryObserver
  {
  private:
    ID2D1SolidColorBrush* pBlackBrush = nullptr;
    IDWriteTextFormat* pTextFormat    = nullptr;
    ID2D1Bitmap* pFolderIcon          = nullptr;

    AllocatorBase* pAllocator = nullptr;
    ArrayList<Entry> entries;
    Allocation searchBuffer;
    Allocation resultsBuffer;

    /// <summary>
    /// Dumb flag variable to check if the Everything query is done
    /// </summary>
    bool queryDone;

    ID2D1Bitmap* ConvertIcon(HICON hIcon);
    void CheckEverythingQueryPrerequisites();

  public:
    void Init(AllocatorBase* pAllocator) override;
    virtual void Paint() override;
    void Destroy() override;

    // Event callbacks
    void OnEverythingQuery();

    virtual void OnID2D1RenderTargetAvailable(ID2D1RenderTarget* pContext) override;
    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) override;
  };
} // namespace mj
