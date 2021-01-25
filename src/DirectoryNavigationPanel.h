#pragma once
#include "Control.h"
#include "mj_common.h"
#include "mj_string.h"
#include "ServiceLocator.h"
#include "Threadpool.h"
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
    StringView* pName;
  };

  namespace detail
  {
    struct ListFolderContentsTask;
    struct CreateTextFormatTask;
    struct LoadFolderIconTask;
    struct LoadFileIconTask;
    struct EverythingQueryContext;
  } // namespace detail

  class DirectoryNavigationPanel : public Control,
                                   public svc::ID2D1RenderTargetObserver,
                                   public svc::IDWriteFactoryObserver
  {
  private:
    friend struct detail::ListFolderContentsTask;
    friend struct detail::CreateTextFormatTask;
    friend struct detail::LoadFolderIconTask;
    friend struct detail::LoadFileIconTask;
    friend struct detail::EverythingQueryContext;

    ID2D1SolidColorBrush* pBlackBrush          = nullptr;
    ID2D1SolidColorBrush* pScrollbarForegroundBrush = nullptr;
    ID2D1SolidColorBrush* pScrollbarBackgroundBrush = nullptr;
    ID2D1SolidColorBrush* pEntryHighlightBrush = nullptr;
    IDWriteTextFormat* pTextFormat             = nullptr;
    ID2D1Bitmap* pFolderIcon                   = nullptr;
    ID2D1Bitmap* pFileIcon                     = nullptr;
    const Entry* pHoveredEntry                 = nullptr;
    StringCache breadcrumb;

    // Open folder
    ArrayList<wchar_t> alOpenFolder;
    StringBuilder sbOpenFolder;

    MJ_UNINITIALIZED D2D1_RECT_F highlightRect;

    AllocatorBase* pAllocator = nullptr;
    ArrayList<Entry> entries;
    int32_t numEntriesDoneLoading = 0;
    Allocation searchBuffer;
    Allocation resultsBuffer;

    // Scrolling
    int16_t mouseWheelAccumulator = 0;
    int32_t scrollOffset          = 0;

    // ListFolderContentsTask results
    struct
    {
      mj::ArrayList<size_t> folders;
      mj::ArrayList<size_t> files;
      mj::StringCache stringCache;
    } listFolderContentsTaskResult;
    detail::ListFolderContentsTask* pListFolderContentsTask = nullptr;

    /// <summary>
    /// Dumb flag variable to check if the Everything query is done
    /// </summary>
    bool queryDone;

    ID2D1Bitmap* ConvertIcon(HICON hIcon);
    void CheckEverythingQueryPrerequisites();
    void TryCreateFolderContentTextLayouts();
    void SetTextLayout(Entry* pEntry, IDWriteTextLayout* pTextLayout);
    void ClearEntries();
    mj::Entry* TestMouseEntry(int16_t x, int16_t y, RECT* pRect);
    void OpenSubFolder(const wchar_t* pFolder);
    void OpenFolder();

    // Event callbacks
    void OnEverythingQuery();
    void OnListFolderContentsDone(detail::ListFolderContentsTask* pTask);
    void OnLoadFolderIconTaskDone(detail::LoadFolderIconTask* pTask);
    void OnLoadFileIconTaskDone(detail::LoadFileIconTask* pTask);

    static constexpr const int16_t entryHeight = 21;

  public:
    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void OnPaint() override;
    virtual void Destroy() override;

    virtual void OnMouseMove(int16_t x, int16_t y) override;
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask) override;
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta) override;
    virtual void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY) override;
    virtual void OnSize() override;

    virtual void OnID2D1RenderTargetAvailable(ID2D1RenderTarget* pContext) override;
    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) override;
  };

  namespace detail
  {
    struct ListFolderContentsTask : public mj::Task
    {
      // In
      MJ_UNINITIALIZED mj::DirectoryNavigationPanel* pParent;
      MJ_UNINITIALIZED mj::StringView directory;

      // Out
      mj::ArrayList<size_t> folders;
      mj::ArrayList<size_t> files;
      mj::StringCache stringCache;

      // Private
      MJ_UNINITIALIZED mj::HeapAllocator allocator;

      virtual void Execute() override;
      virtual void OnDone() override;
      virtual void Destroy() override;

    private:
      bool Add(mj::ArrayList<size_t>& list, size_t index);
    };

    struct CreateTextFormatTask : public mj::Task
    {
      // In
      MJ_UNINITIALIZED mj::DirectoryNavigationPanel* pParent;
      MJ_UNINITIALIZED mj::Entry* pEntry;

      // Out
      MJ_UNINITIALIZED IDWriteTextLayout* pTextLayout;

      virtual void Execute() override;
      virtual void OnDone() override;
      virtual void Destroy() override;
    };

    // Requires: D2D1RenderTarget
    struct LoadFolderIconTask : public mj::Task
    {
      // In
      mj::DirectoryNavigationPanel* pParent = nullptr;
      WORD resource                         = 0;

      // Out
      ID2D1Bitmap* pBitmap = nullptr;

      virtual void Execute() override;
      virtual void OnDone() override;
      virtual void Destroy() override;
    };

    // Requires: D2D1RenderTarget
    struct LoadFileIconTask : public mj::Task
    {
      // In
      mj::DirectoryNavigationPanel* pParent = nullptr;
      WORD resource                         = 0;

      // Out
      ID2D1Bitmap* pBitmap = nullptr;

      virtual void Execute() override;
      virtual void OnDone() override;
      virtual void Destroy() override;
    };

    struct EverythingQueryContext : public mj::Task
    {
      mj::DirectoryNavigationPanel* pParent = nullptr;
      MJ_UNINITIALIZED mj::StringView directory;
      MJ_UNINITIALIZED mj::Allocation searchBuffer;

      virtual void Execute() override;

      virtual void OnDone() override;
    };
  } // namespace detail
} // namespace mj
