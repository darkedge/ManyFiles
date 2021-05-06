#pragma once
#include "Control.h"
#include "mj_common.h"
#include "mj_string.h"
#include "ServiceLocator.h"
#include "Threadpool.h"
#include <d2d1_1.h>
#include "ResourcesD2D1.h"
#include "mj_optional.h"

namespace mj
{
  struct Rect
  {
    MJ_UNINITIALIZED int16_t x;
    MJ_UNINITIALIZED int16_t y;
    MJ_UNINITIALIZED int16_t width;
    MJ_UNINITIALIZED int16_t height;
  };

  struct PopRect
  {
    MJ_UNINITIALIZED D2D1_MATRIX_3X2_F transform;
    void Pop(ID2D1RenderTarget* pRenderTarget);
  };

  PopRect PushRect(ID2D1RenderTarget* pRenderTarget, const Rect& rect);
  PopRect PushRect(ID2D1RenderTarget* pRenderTarget, int16_t x, int16_t y, int16_t width, int16_t height);
} // namespace mj

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
    struct EverythingQueryTask;
  } // namespace detail

  struct DirectoryNavigationPanel : public svc::IDWriteFactoryObserver, //
                                    public res::d2d1::BitmapObserver
  {
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

    /// <summary>
    /// The format of text used for text layouts
    /// </summary>
    IDWriteTextFormat* pTextFormat = nullptr;
    /// <summary>
    /// Points to the last entry in the breadcrumb
    /// </summary>
    IDWriteTextLayout* pCurrentFolderTextLayout = nullptr;
    mj::StringAlloc currentFolderText           = {};
    const Entry* pHoveredEntry                  = nullptr;
    Breadcrumb breadcrumb;

    // Open folder
    StringBuilder sbOpenFolder;

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

    MJ_UNINITIALIZED Rect rect;

    mj::optional<size_t> selectedEntry;

    void Init(AllocatorBase* pAllocator);
    void Paint(ID2D1RenderTarget* pRenderTarget);
    const wchar_t* GetType()
    {
      return MJ_NAMEOF(DirectoryNavigationPanel);
    }
    void Destroy();

    void OnMouseMove(MouseMoveEvent* pMouseMoveEvent);
    void OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask);
    void OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta);
    void OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY);
    void Resize(int16_t x, int16_t y, int16_t width, int16_t height);

    void MoveSelectionUp();
    void MoveSelectionDown();

    virtual void OnIDWriteFactoryAvailable(IDWriteFactory* pFactory) override;
    virtual void OnIconBitmapAvailable(ID2D1Bitmap* pIconBitmap, WORD resource) override;
  };
} // namespace mj
