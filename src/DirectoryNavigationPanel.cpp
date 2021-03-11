#include "DirectoryNavigationPanel.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"
#include <wincodec.h>
#include "Everything.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <shellapi.h>
#include "../3rdparty/tracy/Tracy.hpp"
#include "Threadpool.h"
#include "../vs/resource.h"
#define STRICT_TYPED_ITEMIDS
#include <Shlobj.h>

static float ConvertPointSizeToDIP(float points)
{
  return ((points / 72.0f) * 96.0f);
}

struct InvalidateRectTask : public mj::Task
{
  void Execute() override
  {
    ZoneScoped;
    MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
  }
};

void mj::DirectoryNavigationPanel::Breadcrumb::Init(AllocatorBase* pAllocator)
{
  this->breadcrumb.Init(pAllocator);
}

void mj::DirectoryNavigationPanel::Breadcrumb::Destroy()
{
  this->breadcrumb.Destroy();
}

bool mj::DirectoryNavigationPanel::Breadcrumb::Add(const wchar_t* pStringLiteral)
{
  return this->breadcrumb.Add(pStringLiteral);
}

bool mj::DirectoryNavigationPanel::Breadcrumb::Add(const StringView& string)
{
  return this->breadcrumb.Add(string);
}

mj::StringView* mj::DirectoryNavigationPanel::Breadcrumb::Last()
{
  size_t size = this->breadcrumb.Size();
  if (size > 0)
  {
    return this->breadcrumb[size - 1];
  }

  return nullptr;
}

ID2D1Bitmap* mj::DirectoryNavigationPanel::ConvertIcon(HICON hIcon)
{
  ZoneScoped;
  auto* pFactory = svc::WicFactory();

  MJ_UNINITIALIZED IWICBitmap* pWicBitmap;
  MJ_ERR_HRESULT(pFactory->CreateBitmapFromHICON(hIcon, &pWicBitmap));
  MJ_DEFER(pWicBitmap->Release());

  // Convert format to 32bppPBGRA - which D2D expects.
  MJ_UNINITIALIZED IWICFormatConverter* pConverter;
  MJ_ERR_HRESULT(pFactory->CreateFormatConverter(&pConverter));
  MJ_DEFER(pConverter->Release());

  MJ_ERR_HRESULT(pConverter->Initialize(pWicBitmap,                    //
                                        GUID_WICPixelFormat32bppPBGRA, //
                                        WICBitmapDitherTypeNone,       //
                                        nullptr,                       //
                                        0.0f,                          //
                                        WICBitmapPaletteTypeCustom));

  auto* pRenderTarget = svc::D2D1RenderTarget();
  MJ_UNINITIALIZED ID2D1Bitmap* pBitmap;
  MJ_ERR_HRESULT(pRenderTarget->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap));

  return pBitmap;
}

void mj::detail::EverythingQueryContext::Execute()
{
  ZoneScoped;
  mj::LinearAllocator alloc;
  alloc.Init(this->searchBuffer);

  mj::ArrayList<wchar_t> arrayList;
  arrayList.Init(&alloc, this->searchBuffer.numBytes / sizeof(wchar_t));

  mj::StringBuilder sb;
  sb.SetArrayList(&arrayList);

  mj::StringView search = sb.Append(L"\"")             //
                              .Append(this->directory) //
                              .Append(L"\" !\"")       //
                              .Append(this->directory) //
                              .Append(L"*\\*\"")       //
                              .ToStringClosed();

  Everything_SetSearchW(search.ptr);
  {
    static_cast<void>(Everything_QueryW(TRUE));
  }
}

void mj::detail::EverythingQueryContext::OnDone()
{
  ZoneScoped;
  this->pParent->OnEverythingQuery();
}

void mj::DirectoryNavigationPanel::OnIconBitmapAvailable(ID2D1Bitmap* pIconBitmap, uint16_t resource)
{
  if (resource == IDB_FOLDER)
  {
    for (auto& entry : this->entries)
    {
      if (entry.type == EEntryType::Directory)
      {
        entry.pIcon = pIconBitmap;
        pIconBitmap->AddRef();
      }
    }
    mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
  }
  else if (resource == IDB_DOCUMENT)
  {
    for (auto& entry : this->entries)
    {
      if (entry.type == EEntryType::File)
      {
        entry.pIcon = pIconBitmap;
        pIconBitmap->AddRef();
      }
    }
    mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
  }
}

bool mj::detail::ListFolderContentsTask::Add(mj::ArrayList<size_t>& list, size_t index)
{
  size_t* pItem = list.Emplace(1);

  if (!pItem)
  {
    this->files.Destroy();
    this->folders.Destroy();
    this->stringCache.Destroy();
    return false;
  }

  *pItem = index;

  return true;
}

void mj::detail::ListFolderContentsTask::Execute()
{
  ZoneScoped;

  this->allocator.Init();
  this->files.Init(&this->allocator);
  this->folders.Init(&this->allocator);
  this->stringCache.Init(&this->allocator);
  this->status = 0;

  MJ_UNINITIALIZED WIN32_FIND_DATA findData;
  HANDLE hFind = ::FindFirstFileW(this->directory.ptr, &findData);
  if (hFind == INVALID_HANDLE_VALUE)
  {
    // TODO: Handle ::GetLastError().
    // Example: 0x00000005 --> Access is denied.
    this->status = ::GetLastError();
    return;
  }

  do
  {
    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
    {
      MJ_UNINITIALIZED StringView string;
      string.Init(findData.cFileName);

      // Ignore "." and ".."
      if (string.Equals(L".") || string.Equals(L".."))
      {
        continue;
      }

      if (!this->stringCache.Add(string))
      {
        this->files.Destroy();
        this->folders.Destroy();
        this->stringCache.Destroy();
        break;
      }

      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        if (!this->Add(this->folders, this->stringCache.Size() - 1))
        {
          break;
        }
      }
      else
      {
        if (!this->Add(this->files, this->stringCache.Size() - 1))
        {
          break;
        }
      }
    }
  } while (::FindNextFileW(hFind, &findData) != 0);

  ::FindClose(hFind);
}

void mj::detail::ListFolderContentsTask::OnDone()
{
  ZoneScoped;
  pParent->OnListFolderContentsDone(this);
}

void mj::detail::ListFolderContentsTask::Destroy()
{
  ZoneScoped;
  this->files.Destroy();
  this->folders.Destroy();
  this->stringCache.Destroy();
}

void mj::DirectoryNavigationPanel::OpenSubFolder(const wchar_t* pFolder)
{
  StringView* pLast = this->breadcrumb.Last();
  if (pLast)
  {
    this->sbOpenFolder.Clear();
    this->sbOpenFolder.Append(*pLast);
    this->sbOpenFolder.Append(L"\\");
    this->sbOpenFolder.Append(pFolder);
  }

  this->OpenFolder();
}

void mj::DirectoryNavigationPanel::OpenFolder()
{
  if (this->pListFolderContentsTask)
  {
    this->pListFolderContentsTask->cancelled = true;
  }

  // Note: The StringBuilder should already contain the folder here.
  this->sbOpenFolder.Append(L"\\*");
  this->pListFolderContentsTask            = mj::ThreadpoolCreateTask<mj::detail::ListFolderContentsTask>();
  this->pListFolderContentsTask->pParent   = this;
  this->pListFolderContentsTask->directory = sbOpenFolder.ToStringClosed();
  mj::ThreadpoolSubmitTask(this->pListFolderContentsTask);
}

void mj::DirectoryNavigationPanel::Init(mj::AllocatorBase* pAllocator)
{
  ZoneScoped;
  svc::AddIDWriteFactoryObserver(this);
  res::d2d1::AddBitmapObserver(this);

  // Allocator setup
  this->pAllocator    = pAllocator;
  this->searchBuffer  = this->pAllocator->Allocation(1 * 1024);
  this->resultsBuffer = this->pAllocator->Allocation(1 * 1024 * 1024);
  MJ_EXIT_NULL(this->searchBuffer.pAddress);
  MJ_EXIT_NULL(this->resultsBuffer.pAddress);
  this->entries.Init(this->pAllocator);

  this->listFolderContentsTaskResult.files.Init(this->pAllocator);
  this->listFolderContentsTaskResult.folders.Init(this->pAllocator);
  this->listFolderContentsTaskResult.stringCache.Init(this->pAllocator);

  // FIXME: When opening a folder, add all parent folders to the breadcrumb
  this->breadcrumb.Init(pAllocator);
  this->breadcrumb.Add(L"C:");
  this->alOpenFolder.Init(pAllocator);
  this->sbOpenFolder.SetArrayList(&this->alOpenFolder);

  // Start tasks
  this->sbOpenFolder.Clear();
  this->sbOpenFolder.Append(*this->breadcrumb.Last());
  this->OpenFolder();

#if 0
  {
    EverythingQueryContext* pTask = mj::ThreadpoolCreateTask<EverythingQueryContext>();
    pTask->pParent                = this;
    pTask->directory              = mj::StringView(LR"(C:\*)");
    pTask->searchBuffer           = this->searchBuffer;
    this->queryDone               = false;
    mj::ThreadpoolSubmitTask(pTask);
  }
#endif
}

void mj::DirectoryNavigationPanel::CheckEverythingQueryPrerequisites()
{
  ZoneScoped;
  auto* pFactory = svc::DWriteFactory();
  if (pFactory && queryDone && res::d2d1::FolderIcon() && svc::D2D1RenderTarget())
  {
    // Display results.
    DWORD numResults = Everything_GetNumResults();

    this->ClearEntries();
    Entry* pEntries = this->entries.Emplace(numResults);

    wchar_t fullPathName[MAX_PATH];
    for (DWORD i = 0; i < numResults; i++)
    {
      auto& entry = pEntries[i];

      MJ_UNINITIALIZED StringView string;
      string.Init(Everything_GetResultFileNameW(i));
      MJ_ERR_HRESULT(pFactory->CreateTextLayout(string.ptr,                      //
                                                static_cast<UINT32>(string.len), //
                                                this->pTextFormat,               //
                                                1024.0f,                         //
                                                1024.0f,                         //
                                                &entry.pTextLayout));

      // Get associated file icon
      if (Everything_IsFileResult(i))
      {
        MJ_UNINITIALIZED SHFILEINFO fileInfo;
        Everything_GetResultFullPathNameW(i, fullPathName, MJ_COUNTOF(fullPathName));
        {
          ZoneScopedN("SHGetFileInfoW");
          MJ_ERR_ZERO(::SHGetFileInfoW(fullPathName, 0, &fileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON));
        }
        MJ_DEFER(::DestroyIcon(fileInfo.hIcon));
        entry.pIcon = this->ConvertIcon(fileInfo.hIcon);
      }
      else if (Everything_IsFolderResult(i))
      {
        entry.pIcon = res::d2d1::FolderIcon();
      }
    }
  }
  mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
}

void mj::DirectoryNavigationPanel::Paint(ID2D1RenderTarget* pRenderTarget)
{
  // Draw scrollbar
  if (this->height > 0 && this->entries.Size() > 0)
  {
    FLOAT pixelHeight = static_cast<FLOAT>(this->entries.Size()) * this->entryHeight;
    FLOAT viewHeight  = this->height;
    if (pixelHeight > viewHeight)
    {
      FLOAT top = -this->scrollOffset * viewHeight / pixelHeight;
      {
        D2D1_RECT_F rect = D2D1::RectF( //
            this->width - 16.0f,        //
            0,                          //
            this->width,                //
            viewHeight);
        pRenderTarget->FillRectangle(rect, res::d2d1::ScrollbarBackgroundBrush());
      }
      {
        D2D1_RECT_F rect = D2D1::RectF( //
            this->width - 16.0f,        //
            top,                        //
            this->width,                //
            top + viewHeight * viewHeight / pixelHeight);
        pRenderTarget->FillRectangle(rect, res::d2d1::ScrollbarForegroundBrush());
      }
    }
  }

  auto point = D2D1::Point2F(16.0f, static_cast<FLOAT>(this->scrollOffset));

  if (this->pCurrentFolderTextLayout)
  {
    pRenderTarget->DrawTextLayout(point, this->pCurrentFolderTextLayout, res::d2d1::BlackBrush());
  }
  point.y += this->entryHeight;

  for (const auto& entry : this->entries)
  {
    if (entry.pTextLayout)
    {
      pRenderTarget->DrawTextLayout(point, entry.pTextLayout, res::d2d1::BlackBrush());
    }

    if (entry.type == EEntryType::Directory && res::d2d1::FolderIcon())
    {
      auto iconSize = res::d2d1::FolderIcon()->GetPixelSize();
      float width   = static_cast<float>(iconSize.width);
      float height  = static_cast<float>(iconSize.height);
      pRenderTarget->DrawBitmap(res::d2d1::FolderIcon(), D2D1::RectF(0.0f, point.y, width, point.y + height));
    }
    else if (entry.pIcon)
    {
      auto iconSize = entry.pIcon->GetPixelSize();
      float width   = static_cast<float>(iconSize.width);
      float height  = static_cast<float>(iconSize.height);
      pRenderTarget->DrawBitmap(entry.pIcon, D2D1::RectF(0.0f, point.y, width, point.y + height));
    }

    // Always draw images on integer coordinates
    point.y += this->entryHeight;
  }

  if (this->pHoveredEntry && res::d2d1::EntryHighlightBrush())
  {
    pRenderTarget->DrawRectangle(&this->highlightRect, res::d2d1::EntryHighlightBrush());
  }
}

void mj::DirectoryNavigationPanel::Destroy()
{
  ZoneScoped;

  this->breadcrumb.Destroy();
  this->alOpenFolder.Destroy();

  this->pAllocator->Free(this->searchBuffer.pAddress);
  this->pAllocator->Free(this->resultsBuffer.pAddress);

  this->ClearEntries();
  this->entries.Destroy();

  this->listFolderContentsTaskResult.files.Destroy();
  this->listFolderContentsTaskResult.folders.Destroy();
  this->listFolderContentsTaskResult.stringCache.Destroy();

  if (this->pListFolderContentsTask)
  {
    this->pListFolderContentsTask->cancelled = true;
    this->pListFolderContentsTask            = nullptr;
  }

  svc::RemoveIDWriteFactoryObserver(this);
  res::d2d1::RemoveBitmapObserver(this);
}

mj::Entry* mj::DirectoryNavigationPanel::TestMouseEntry(int16_t x, int16_t y, RECT* pRect)
{
  auto point = D2D1::Point2F(16.0f, static_cast<FLOAT>(this->scrollOffset));
  for (auto i = 0; i < this->entries.Size(); i++)
  {
    auto& entry = this->entries[i];
    if (entry.pTextLayout)
    {
      MJ_UNINITIALIZED DWRITE_TEXT_METRICS metrics;
      // This call costs nothing, looks like it's just a copy
      // Optimization note: we could extract this information on creation
      // and put the rect in a separate list for faster iteration.
      MJ_ERR_HRESULT(entry.pTextLayout->GetMetrics(&metrics));

      MJ_UNINITIALIZED RECT rect;
      rect.left   = static_cast<LONG>(point.x + metrics.left);
      rect.right  = static_cast<LONG>(point.x + metrics.left + metrics.width);
      rect.top    = static_cast<LONG>(point.y + metrics.top);
      rect.bottom = static_cast<LONG>(point.y + metrics.top + metrics.height);

      MJ_UNINITIALIZED POINT p;
      p.x = x;
      p.y = y;

      if (::PtInRect(&rect, p))
      {
        if (pRect)
        {
          *pRect = rect;
        }
        return &entry;
      }
    }
    point.y += 21;
  }

  return nullptr;
}

void mj::DirectoryNavigationPanel::OnMouseMove(MouseMoveEvent* pMouseMoveEvent)
{
  auto pHoveredPrev   = this->pHoveredEntry;
  this->pHoveredEntry = nullptr;

  MJ_UNINITIALIZED RECT rect;
  mj::Entry* pEntry = this->TestMouseEntry(pMouseMoveEvent->x, pMouseMoveEvent->y, &rect);
  if (pEntry)
  {
    this->pHoveredEntry        = pEntry;
    this->highlightRect.left   = static_cast<FLOAT>(rect.left);
    this->highlightRect.right  = static_cast<FLOAT>(rect.right);
    this->highlightRect.top    = static_cast<FLOAT>(rect.top);
    this->highlightRect.bottom = static_cast<FLOAT>(rect.bottom);
  }

  if (pHoveredPrev != this->pHoveredEntry)
  {
    mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
  }
}

void mj::DirectoryNavigationPanel::OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
{
  static_cast<void>(mkMask);

  mj::Entry* pEntry = this->TestMouseEntry(x, y, nullptr);
  if (pEntry)
  {
    if (pEntry->type == EEntryType::Directory)
    {
      this->OpenSubFolder(pEntry->pName->ptr);
    }
  }
}

void mj::DirectoryNavigationPanel::OnMouseWheel(int16_t x, int16_t y, uint16_t mkMask, int16_t zDelta)
{
  static_cast<void>(x);
  static_cast<void>(y);
  static_cast<void>(mkMask);

  this->mouseWheelAccumulator += zDelta;

  // Reset if switching directions
  if (this->mouseWheelAccumulator < 0 && zDelta > 0 || //
      this->mouseWheelAccumulator > 0 && zDelta < 0)
  {
    this->mouseWheelAccumulator = zDelta;
  }

  int16_t numScrolls = this->mouseWheelAccumulator / WHEEL_DELTA;
  if (this->mouseWheelAccumulator != 0)
  {
    this->mouseWheelAccumulator -= numScrolls * WHEEL_DELTA;
  }

  MJ_UNINITIALIZED UINT pvParam;
  // 7-15 microseconds
  MJ_ERR_IF(::SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &pvParam, 0), 0);

  int16_t diff = numScrolls * static_cast<int16_t>(pvParam) * this->entryHeight;
  if (diff != 0)
  {
    this->scrollOffset += diff;
    int32_t pixelHeight = static_cast<int32_t>(this->entries.Size()) * this->entryHeight;
    if (this->scrollOffset > 0 || pixelHeight < this->height)
    {
      this->scrollOffset = 0;
    }
    else if (this->scrollOffset + pixelHeight < this->height)
    {
      this->scrollOffset = this->height - pixelHeight;
    }

    mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
  }
}

void mj::DirectoryNavigationPanel::OnContextMenu(int16_t clientX, int16_t clientY, int16_t screenX, int16_t screenY)
{
  mj::Entry* pEntry = this->TestMouseEntry(clientX, clientY, nullptr);
  if (pEntry)
  {
    if (pEntry->type == EEntryType::Directory)
    {
      ZoneScoped;

      MJ_UNINITIALIZED IShellFolder* pDesktop;
      MJ_ERR_HRESULT(::SHGetDesktopFolder(&pDesktop));
      MJ_DEFER(pDesktop->Release());

      StringView* pLast = this->breadcrumb.Last();
      if (pLast)
      {
        this->sbOpenFolder.Clear();
        this->sbOpenFolder.Append(*pLast);
        this->sbOpenFolder.Append(L"\\");
        this->sbOpenFolder.Append(*pEntry->pName);

        auto path = this->sbOpenFolder.ToStringClosed();
        MJ_UNINITIALIZED PIDLIST_RELATIVE pidl;
        MJ_ERR_HRESULT(pDesktop->ParseDisplayName(nullptr,                        //
                                                  nullptr,                        //
                                                  const_cast<wchar_t*>(path.ptr), // Why is this not const?
                                                  nullptr,                        //
                                                  &pidl,                          //
                                                  nullptr));
        MJ_DEFER(::CoTaskMemFree(pidl));

        MJ_UNINITIALIZED IContextMenu* pContextMenu;
        MJ_ERR_HRESULT(pDesktop->GetUIObjectOf(nullptr,                                        //
                                               1,                                              //
                                               reinterpret_cast<PCUITEMID_CHILD_ARRAY>(&pidl), //
                                               IID_IContextMenu,                               //
                                               nullptr,                                        //
                                               reinterpret_cast<void**>(&pContextMenu)));

        MJ_UNINITIALIZED HMENU pMenu;
        MJ_ERR_IF(pMenu = ::CreatePopupMenu(), nullptr);
        MJ_DEFER(::DestroyMenu(pMenu));

        // Fill context menu
        {
          // Slow operation (100s of milliseconds), but part of critical path
          ZoneScopedN("QueryContextMenu");

          // If successful, returns an HRESULT value that has its severity value set to SEVERITY_SUCCESS and its code
          // value set to the offset of the largest command identifier that was assigned, plus one. For example, if
          // idCmdFirst is set to 5 and you add three items to the menu with command identifiers of 5, 7, and 8, the
          // return value should be MAKE_HRESULT(SEVERITY_SUCCESS, 0, 8 - 5 + 1). Otherwise, it returns a COM error
          // value.
          static_cast<void>(pContextMenu->QueryContextMenu(pMenu, 0, 1, 0x7fff, CMF_NORMAL | CMF_EXPLORE));
        }

        // This is one of those instances where a BOOL can have many values
        MJ_UNINITIALIZED BOOL cmd;
        {
          // Blocking until popup menu is closed
          ZoneScopedN("TrackPopupMenu");

          // If you specify TPM_RETURNCMD in the uFlags parameter, the return value is the menu-item identifier of the
          // item that the user selected. If the user cancels the menu without making a selection, or if an error
          // occurs, the return value is zero. If you do not specify TPM_RETURNCMD in the uFlags parameter, the return
          // value is nonzero if the function succeeds and zero if it fails. To get extended error information, call
          // GetLastError.
          cmd = ::TrackPopupMenu(pMenu,                   //
                                 TPM_RETURNCMD,           //
                                 screenX,                 //
                                 screenY,                 //
                                 0,                       //
                                 svc::MainWindowHandle(), //
                                 nullptr);
        }

        if (cmd)
        {
          CMINVOKECOMMANDINFOEX info = {};
          info.cbSize                = sizeof(info);
          info.fMask                 = CMIC_MASK_UNICODE;
          info.hwnd                  = svc::MainWindowHandle();
          info.lpVerb                = MAKEINTRESOURCEA(cmd - 1);
          info.lpVerbW               = MAKEINTRESOURCEW(cmd - 1);
          info.nShow                 = SW_SHOWNORMAL;
          MJ_ERR_HRESULT(pContextMenu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&info)));
        }
      }
    }
  }
}

void mj::DirectoryNavigationPanel::OnSize()
{
  this->mouseWheelAccumulator = 0;
  this->scrollOffset          = 0;
}

void mj::DirectoryNavigationPanel::OnEverythingQuery()
{
  queryDone = true;
  this->CheckEverythingQueryPrerequisites();
}

void mj::DirectoryNavigationPanel::OnListFolderContentsDone(detail::ListFolderContentsTask* pTask)
{
  if (pTask->status == 0 &&                                              //
      this->listFolderContentsTaskResult.files.Copy(pTask->files) &&     //
      this->listFolderContentsTaskResult.folders.Copy(pTask->folders) && //
      this->listFolderContentsTaskResult.stringCache.Copy(pTask->stringCache))
  {
    // FIXME: This can trigger a reallocation so if we use the breadcrumb elsewhere we're screwed
    StringView str = this->sbOpenFolder.ToStringOpen();
    str.Init(str.ptr, str.FindLastOf(L"\\*"));

    this->breadcrumb.Add(str);

    // TODO: Start icon, TextFormat tasks if preconditions are met
    // this->TryLoadFolderContentIcons();
    this->TryCreateFolderContentTextLayouts();
  }
  this->pListFolderContentsTask = nullptr;
}

void mj::detail::CreateTextLayoutTask::Execute()
{
  ZoneScoped;

  MJ_ERR_HRESULT(svc::DWriteFactory()->CreateTextLayout(this->pEntry->pName->ptr,                      //
                                                        static_cast<UINT32>(this->pEntry->pName->len), //
                                                        pParent->pTextFormat,                          //
                                                        1024.0f,                                       //
                                                        1024.0f,                                       //
                                                        &this->pTextLayout));

  // FIXME: If this task is slow, InvalidateRect does not show everything...
  // ::Sleep(1000);
}

void mj::detail::CreateTextLayoutTask::OnDone()
{
  ZoneScoped;
  this->pTextLayout->AddRef();
  this->pParent->SetTextLayout(this->pEntry, this->pTextLayout);
}

void mj::detail::CreateTextLayoutTask::Destroy()
{
  this->pTextLayout->Release();
}

void mj::DirectoryNavigationPanel::SetTextLayout(mj::Entry* pEntry, IDWriteTextLayout* pTextLayout)
{
  pEntry->pTextLayout = pTextLayout;

  if (++this->numEntriesDoneLoading == this->entries.Size())
  {
    this->scrollOffset = 0;
    mj::ThreadpoolSubmitTask(mj::ThreadpoolCreateTask<InvalidateRectTask>());
  }
}

void mj::DirectoryNavigationPanel::TryCreateFolderContentTextLayouts()
{
  ZoneScoped;

  // Note: If the folder is empty, we do nothing.
  // This is okay if we don't want to render anything, but this could change.
  auto numItems = this->listFolderContentsTaskResult.stringCache.Size();

  // Skipping the check for DWrite because our TextFormat already depends on it.
  if (numItems > 0 && this->pTextFormat)
  {
    this->ClearEntries();
    this->pHoveredEntry = nullptr;
    if (this->entries.Emplace(numItems))
    {
      // Variable number of tasks with the same cancellation token
      this->numEntriesDoneLoading = 0;

      // Folders
      for (auto i = 0; i < this->listFolderContentsTaskResult.folders.Size(); i++)
      {
        auto& entry = this->entries[i];
        entry       = {};
        entry.type  = EEntryType::Directory;
        entry.pName = this->listFolderContentsTaskResult.stringCache[this->listFolderContentsTaskResult.folders[i]];
        entry.pIcon = res::d2d1::FolderIcon();

        auto pTask     = mj::ThreadpoolCreateTask<mj::detail::CreateTextLayoutTask>();
        pTask->pParent = this;
        pTask->pEntry  = &entry;
        mj::ThreadpoolSubmitTask(pTask);
      }

      // Files
      for (auto i = 0; i < this->listFolderContentsTaskResult.files.Size(); i++)
      {
        auto& entry = this->entries[i + this->listFolderContentsTaskResult.folders.Size()];
        entry       = {};
        entry.type  = EEntryType::File;
        entry.pName = this->listFolderContentsTaskResult.stringCache[this->listFolderContentsTaskResult.files[i]];
        entry.pIcon = res::d2d1::FileIcon();

        auto pTask     = mj::ThreadpoolCreateTask<mj::detail::CreateTextLayoutTask>();
        pTask->pParent = this;
        pTask->pEntry  = &entry;
        mj::ThreadpoolSubmitTask(pTask);
      }
    }
  }
}

void mj::DirectoryNavigationPanel::ClearEntries()
{
  for (auto& element : this->entries)
  {
    // Only release if icon exists and is not a shared icon
    if (element.pIcon && element.pIcon != res::d2d1::FolderIcon() && element.pIcon != res::d2d1::FileIcon())
    {
      element.pIcon->Release();
    }
    if (element.pTextLayout)
    {
      element.pTextLayout->Release();
    }
  }
  this->entries.Clear();
}

void mj::DirectoryNavigationPanel::OnIDWriteFactoryAvailable(IDWriteFactory* pFactory)
{
  ZoneScoped;
  MJ_SAFE_RELEASE(this->pTextFormat);
  MJ_ERR_HRESULT(pFactory->CreateTextFormat(L"Segoe UI",                   // Font name
                                            nullptr,                       // Font collection
                                            DWRITE_FONT_WEIGHT_NORMAL,     // Font weight
                                            DWRITE_FONT_STYLE_NORMAL,      // Font style
                                            DWRITE_FONT_STRETCH_NORMAL,    // Font stretch
                                            ::ConvertPointSizeToDIP(9.0f), // Font size
                                            L"",                           // Locale name
                                            &this->pTextFormat));
  // this->CheckEverythingQueryPrerequisites();
}
