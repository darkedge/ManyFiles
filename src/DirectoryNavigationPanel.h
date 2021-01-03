#pragma once
#include "IControl.h"
#include "mj_common.h"
#include "StringBuilder.h"
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
    EEntryType type;
    IDWriteTextLayout* pTextLayout;
    ID2D1Bitmap1* pIcon;
  };

  class DirectoryNavigationPanel : public IControl, public svc::IWICFactoryObserver, public svc::ID2D1DeviceContextObserver, public svc::IDWriteFactoryObserver
  {
  private:
    ID2D1SolidColorBrush* pBlackBrush;
    IDWriteTextFormat* pTextFormat;
    ID2D1Bitmap1* pFolderIcon;
    HICON pFolderIconHandle;

    AllocatorBase* pAllocator;
    ArrayList<Entry> entries;
    Allocation searchBuffer;
    Allocation resultsBuffer;

    /// <summary>
    /// Dumb flag variable to check if the Everything query is done
    /// </summary>
    bool queryDone;

    ID2D1Bitmap1* ConvertIcon(HICON hIcon);
    void CheckFolderIconPrerequisites();
    void CheckEverythingQueryPrerequisites();

  public:
    void Init(AllocatorBase* pAllocator) override;
    virtual void Paint() override;
    void Destroy() override;

    // Event callbacks
    void OnLoadFolderIcon(HICON hIcon);
    void OnEverythingQuery();

    virtual void OnWICFactoryAvailable(IWICImagingFactory* pFactory) override;
    virtual void OnID2D1DeviceContextAvailable(ID2D1DeviceContext* pContext) override;
    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) override;
  };
} // namespace mj
