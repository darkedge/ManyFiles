#pragma once
#include "Control.h"
#include "mj_common.h"
#include "mj_string.h"
#include "ServiceLocator.h"
#include "Threadpool.h"
#include <d2d1_1.h>
#include "ResourcesD2D1.h"

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
    struct CreateTextLayoutTask;
    struct LoadFolderIconTask;
    struct LoadFileIconTask;
    struct EverythingQueryContext;
  } // namespace detail

  class DirectoryNavigationPanel : public Control,                     //
                                   public svc::IDWriteFactoryObserver, //
                                   public res::d2d1::BitmapObserver
  {
  private:
    class Breadcrumb
    {
    private:
      StringCache breadcrumb;

    public:
      /// <summary>
      /// Does no allocation on construction.
      /// </summary>
      /// <returns></returns>
      void Init(AllocatorBase* pAllocator);

      /// <summary>
      /// Data is freed using the assigned allocator.
      /// </summary>
      void Destroy();

      bool Add(const wchar_t* pStringLiteral);

bool Add(const StringView& string);

      StringView* Last();
    };

    friend struct detail::ListFolderContentsTask;
    friend struct detail::CreateTextLayoutTask;
    friend struct detail::LoadFolderIconTask;
    friend struct detail::LoadFileIconTask;
    friend struct detail::EverythingQueryContext;

    /// <summary>
    /// The format of text used for text layouts
    /// </summary>
    IDWriteTextFormat* pTextFormat = nullptr;
    /// <summary>
    /// Points to the last entry in the breadcrumb
    /// </summary>
    IDWriteTextLayout* pCurrentFolderTextLayout = nullptr;
    const Entry* pHoveredEntry                  = nullptr;
    Breadcrumb breadcrumb;

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

    static constexpr const int16_t entryHeight = 21;

  public:
    virtual void Init(AllocatorBase* pAllocator) override;
    virtual void Paint(ID2D1RenderTarget* pRenderTarget) override;
    virtual void Destroy() override;

    virtual void OnMouseMove(int16_t x, int16_t y) override;
    virtual void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask) override;
    virtual void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta) override;
    virtual void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY) override;
    virtual void OnSize() override;

    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) override;
    virtual void OnIconBitmapAvailable(ID2D1Bitmap* pIconBitmap, WORD resource) override;
  };

  namespace detail
  {
    struct ListFolderContentsTask : public mj::Task
    {
      // In
      MJ_UNINITIALIZED mj::DirectoryNavigationPanel* pParent;
      MJ_UNINITIALIZED mj::StringView directory;

      // Out
      MJ_UNINITIALIZED HRESULT status;
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

    struct CreateTextLayoutTask : public mj::Task
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
