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
#include "stb_image.h"

static float ConvertPointSizeToDIP(float points)
{
  return ((points / 72.0f) * 96.0f);
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
                              .ToStringNullTerminated();

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

namespace mj
{
  namespace detail
  {
    /// <summary>
    /// Separated function for possible later use
    /// </summary>
    void LoadBitmapFromResource(DWORD resource, ID2D1Bitmap** ppBitmap)
    {
      auto* pRenderTarget = svc::D2D1RenderTarget();
      // Render target should have been assigned
      // before starting tasks that call this function
      // Should we make this a parameter?
      MJ_EXIT_NULL(pRenderTarget);

      // Resource management.
      HGLOBAL imageResDataHandle = nullptr;
      void* pImageFile           = nullptr;

      // Locate the resource in the application's executable.
      HRSRC imageResHandle = ::FindResourceW(nullptr, MAKEINTRESOURCEW(resource), L"PNG");
      MJ_ERR_IF(imageResHandle, nullptr);

      // Load the resource to the HGLOBAL.
      {
        ZoneScopedN("LoadResource");
        imageResDataHandle = ::LoadResource(nullptr, imageResHandle);
        MJ_ERR_IF(imageResDataHandle, nullptr);
      }

      auto size = ::SizeofResource(nullptr, imageResHandle);

      // Lock the resource to retrieve memory pointer.
      {
        ZoneScopedN("LockResource");
        pImageFile = ::LockResource(imageResDataHandle);
        // Does not specify GetLastError
        MJ_EXIT_NULL(pImageFile);
      }

      int numComponents = 4;
      MJ_UNINITIALIZED int x, y, n;
      // ... process data if not NULL ...
      // ... x = width, y = height, n = # 8-bit components per pixel ...
      // ... replace '0' with '1'..'4' to force that many components per pixel
      // ... but 'n' will always be the number that it would have been if you said 0
      uint32_t* data = reinterpret_cast<uint32_t*>(
          ::stbi_load_from_memory(static_cast<stbi_uc*>(pImageFile), size, &x, &y, &n, numComponents));
      MJ_DEFER(::stbi_image_free(data));

      MJ_ERR_HRESULT(pRenderTarget->CreateBitmap(
          D2D1::SizeU(x, y), data, x * numComponents,
          D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
          ppBitmap));
    }
  } // namespace detail
} // namespace mj

void mj::detail::LoadFolderIconTask::Execute()
{
  ZoneScoped;
  mj::detail::LoadBitmapFromResource(this->resource, &this->pBitmap);
}

void mj::detail::LoadFolderIconTask::OnDone()
{
  ZoneScoped;
  this->pBitmap->AddRef();
  this->pParent->OnLoadFolderIconTaskDone(this);
}

void mj::detail::LoadFolderIconTask::Destroy()
{
  ZoneScoped;
  this->pBitmap->Release();
  this->pBitmap = nullptr;
}

void mj::detail::LoadFileIconTask::Execute()
{
  ZoneScoped;
  mj::detail::LoadBitmapFromResource(this->resource, &this->pBitmap);
}

void mj::detail::LoadFileIconTask::OnDone()
{
  ZoneScoped;
  this->pBitmap->AddRef();
  this->pParent->OnLoadFileIconTaskDone(this);
}

void mj::detail::LoadFileIconTask::Destroy()
{
  ZoneScoped;
  this->pBitmap->Release();
  this->pBitmap = nullptr;
}

void mj::DirectoryNavigationPanel::OnLoadFolderIconTaskDone(mj::detail::LoadFolderIconTask* pTask)
{
  this->pFolderIcon = pTask->pBitmap;
  for (auto& entry : this->entries)
  {
    if (entry.type == EEntryType::Directory)
    {
      entry.pIcon = this->pFolderIcon;
    }
  }
  MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
}

void mj::DirectoryNavigationPanel::OnLoadFileIconTaskDone(mj::detail::LoadFileIconTask* pTask)
{
  this->pFileIcon = pTask->pBitmap;
  for (auto& entry : this->entries)
  {
    if (entry.type == EEntryType::File)
    {
      entry.pIcon = this->pFileIcon;
    }
  }
  MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
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

  MJ_UNINITIALIZED WIN32_FIND_DATA findData;
  HANDLE hFind = ::FindFirstFileW(this->directory.ptr, &findData);
  MJ_ERR_IF(hFind, INVALID_HANDLE_VALUE);

  do
  {
    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
    {
      if (!this->stringCache.Add(findData.cFileName))
      {
        this->files.Destroy();
        this->folders.Destroy();
        this->stringCache.Destroy();
        break;
      }

      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
        if (!Add(this->folders, this->stringCache.Size() - 1))
        {
          break;
        }
      }
      else
      {
        if (!Add(this->files, this->stringCache.Size() - 1))
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
  this->files.Destroy();
  this->folders.Destroy();
  this->stringCache.Destroy();
}

void mj::DirectoryNavigationPanel::OpenSubFolder(const wchar_t* pFolder)
{
  auto breadcrumbSize = this->breadcrumb.Size();
  if (breadcrumbSize > 0)
  {
    auto currentPath = this->breadcrumb[breadcrumbSize - 1];
    this->sbOpenFolder.Clear();
    this->sbOpenFolder.Append(currentPath);
    this->sbOpenFolder.Append(L"\\");
    this->sbOpenFolder.Append(pFolder);

    // FIXME: This can trigger a reallocation so if we use the breadcrumb elsewhere we're screwed
    this->breadcrumb.Add(this->sbOpenFolder.ToString());
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
  this->pListFolderContentsTask->directory = sbOpenFolder.ToStringNullTerminated();
  mj::ThreadpoolSubmitTask(this->pListFolderContentsTask);
}

void mj::DirectoryNavigationPanel::Init(mj::AllocatorBase* pAllocator)
{
  ZoneScoped;
  svc::AddID2D1RenderTargetObserver(this);
  svc::AddIDWriteFactoryObserver(this);

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

  this->breadcrumb.Init(pAllocator);
  this->breadcrumb.Add(L"C:");
  this->alOpenFolder.Init(pAllocator);
  this->sbOpenFolder.SetArrayList(&this->alOpenFolder);

  // Start tasks
  this->sbOpenFolder.Clear();
  this->sbOpenFolder.Append(this->breadcrumb[0]);
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
  if (pFactory && queryDone && this->pFolderIcon && svc::D2D1RenderTarget())
  {
    // Display results.
    DWORD numResults = Everything_GetNumResults();

    this->entries.Clear();
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
        entry.pIcon = this->pFolderIcon;
      }
    }
  }
  MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
}

void mj::DirectoryNavigationPanel::OnPaint()
{
  ZoneScoped;
  auto* pRenderTarget = svc::D2D1RenderTarget();

  // Implies that our brushes are also created
  if (pRenderTarget)
  {
    auto point = D2D1::Point2F(16.0f, this->scrollOffset);

    if (this->height > 0)
    {
      auto pixelHeight = this->entries.Size() * this->entryHeight;
      auto viewHeight  = this->height;
      auto top         = -this->scrollOffset * viewHeight / pixelHeight;
      auto rect        = D2D1::RectF( //
          this->width - 16.0f, //
          top,                 //
          this->width,         //
          top + viewHeight * viewHeight / pixelHeight);
      pRenderTarget->DrawRectangle(rect, this->pBlackBrush);
    }

    for (const auto& entry : this->entries)
    {
      if (entry.pTextLayout)
      {
        pRenderTarget->DrawTextLayout(point, entry.pTextLayout, this->pBlackBrush);
      }

      if (entry.type == EEntryType::Directory && this->pFolderIcon)
      {
        auto iconSize = this->pFolderIcon->GetPixelSize();
        float width   = static_cast<float>(iconSize.width);
        float height  = static_cast<float>(iconSize.height);
        pRenderTarget->DrawBitmap(this->pFolderIcon, D2D1::RectF(0.0f, point.y, width, point.y + height));
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

    if (this->pHoveredEntry && this->pEntryHighlightBrush)
    {
      pRenderTarget->DrawRectangle(&this->highlightRect, this->pEntryHighlightBrush);
    }
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

  if (this->pBlackBrush)
  {
    this->pBlackBrush->Release();
    this->pBlackBrush = nullptr;
  }

  if (this->pEntryHighlightBrush)
  {
    this->pEntryHighlightBrush->Release();
    this->pEntryHighlightBrush = nullptr;
  }

  if (this->pFolderIcon)
  {
    this->pFolderIcon->Release();
    this->pFolderIcon = nullptr;
  }
  if (this->pFileIcon)
  {
    this->pFileIcon->Release();
    this->pFileIcon = nullptr;
  }

  this->listFolderContentsTaskResult.files.Destroy();
  this->listFolderContentsTaskResult.folders.Destroy();
  this->listFolderContentsTaskResult.stringCache.Destroy();

  if (this->pListFolderContentsTask)
  {
    this->pListFolderContentsTask->cancelled = true;
    this->pListFolderContentsTask            = nullptr;
  }

  svc::RemoveID2D1RenderTargetObserver(this);
  svc::RemoveIDWriteFactoryObserver(this);
}

bool mj::DirectoryNavigationPanel::TestMouseEntry(int16_t x, int16_t y, mj::Entry** ppEntry, RECT* pRect)
{
  auto point = D2D1::Point2F(16.0f, 0.0f);
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
        if (ppEntry)
        {
          *ppEntry = &entry;
        }
        return true;
      }
    }
    point.y += 21;
  }

  return false;
}

void mj::DirectoryNavigationPanel::OnMouseMove(int16_t x, int16_t y)
{
  auto pHoveredPrev   = this->pHoveredEntry;
  this->pHoveredEntry = nullptr;

  MJ_UNINITIALIZED mj::Entry* pEntry;
  MJ_UNINITIALIZED RECT rect;
  if (this->TestMouseEntry(x, y, &pEntry, &rect))
  {
    this->pHoveredEntry        = pEntry;
    this->highlightRect.left   = static_cast<FLOAT>(rect.left);
    this->highlightRect.right  = static_cast<FLOAT>(rect.right);
    this->highlightRect.top    = static_cast<FLOAT>(rect.top);
    this->highlightRect.bottom = static_cast<FLOAT>(rect.bottom);
  }

  if (pHoveredPrev != this->pHoveredEntry)
  {
    // TODO: This should be global
    ::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE);
  }
}

void mj::DirectoryNavigationPanel::OnDoubleClick(int16_t x, int16_t y, uint16_t mkMask)
{
  MJ_UNINITIALIZED mj::Entry* pEntry;
  if (this->TestMouseEntry(x, y, &pEntry, nullptr))
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
  if (this->mouseWheelAccumulator > 0)
  {
    this->mouseWheelAccumulator -= numScrolls * WHEEL_DELTA;
  }
  else if (this->mouseWheelAccumulator < 0)
  {
    this->mouseWheelAccumulator += numScrolls * WHEEL_DELTA;
  }

  MJ_UNINITIALIZED UINT pvParam;
  {
    ZoneScoped;
    MJ_ERR_IF(::SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &pvParam, 0), 0);
  }

  int16_t diff = numScrolls * pvParam * this->entryHeight;
  if (diff != 0)
  {
    this->scrollOffset += diff;
    int16_t pixelHeight = this->entries.Size() * this->entryHeight;
    if (this->scrollOffset > 0)
    {
      this->scrollOffset = 0;
    }
    else if (this->scrollOffset + pixelHeight < this->height)
    {
      this->scrollOffset = this->height - pixelHeight;
    }

    MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
  }
}

void mj::DirectoryNavigationPanel::OnEverythingQuery()
{
  queryDone = true;
  this->CheckEverythingQueryPrerequisites();
}

void mj::DirectoryNavigationPanel::OnListFolderContentsDone(detail::ListFolderContentsTask* pTask)
{
  if (this->listFolderContentsTaskResult.files.Copy(pTask->files) &&
      this->listFolderContentsTaskResult.folders.Copy(pTask->folders) &&
      this->listFolderContentsTaskResult.stringCache.Copy(pTask->stringCache))
  {
    // TODO: Start icon, TextFormat tasks if preconditions are met
    // this->TryLoadFolderContentIcons();
    this->TryCreateFolderContentTextLayouts();
  }
  this->pListFolderContentsTask = nullptr;
}

void mj::detail::CreateTextFormatTask::Execute()
{
  ZoneScoped;

  MJ_ERR_HRESULT(svc::DWriteFactory()->CreateTextLayout(this->pEntry->pName->ptr,                      //
                                                        static_cast<UINT32>(this->pEntry->pName->len), //
                                                        pParent->pTextFormat,                          //
                                                        1024.0f,                                       //
                                                        1024.0f,                                       //
                                                        &this->pTextLayout));
}

void mj::detail::CreateTextFormatTask::OnDone()
{
  ZoneScoped;
  this->pTextLayout->AddRef();
  this->pParent->SetTextLayout(this->pEntry, this->pTextLayout);
}

void mj::DirectoryNavigationPanel::SetTextLayout(mj::Entry* pEntry, IDWriteTextLayout* pTextLayout)
{
  pEntry->pTextLayout = pTextLayout;

  if (++this->numEntriesDoneLoading == this->entries.Size())
  {
    MJ_ERR_ZERO(::InvalidateRect(svc::MainWindowHandle(), nullptr, FALSE));
  }
}

void mj::detail::CreateTextFormatTask::Destroy()
{
  this->pTextLayout->Release();
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
    if (this->entries.Emplace(numItems))
    {
      // Variable number of tasks with the same cancellation token
      numEntriesDoneLoading = 0;

      // Folders
      for (auto i = 0; i < this->listFolderContentsTaskResult.folders.Size(); i++)
      {
        auto& entry = this->entries[i];
        entry       = {};
        entry.type  = EEntryType::Directory;
        entry.pName = &this->listFolderContentsTaskResult.stringCache[this->listFolderContentsTaskResult.folders[i]];
        entry.pIcon = this->pFolderIcon;

        auto pTask     = mj::ThreadpoolCreateTask<mj::detail::CreateTextFormatTask>();
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
        entry.pName = &this->listFolderContentsTaskResult.stringCache[this->listFolderContentsTaskResult.files[i]];
        entry.pIcon = this->pFileIcon;

        auto pTask     = mj::ThreadpoolCreateTask<mj::detail::CreateTextFormatTask>();
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
    if (element.pIcon && element.pIcon != this->pFolderIcon && element.pIcon != this->pFileIcon)
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

void mj::DirectoryNavigationPanel::OnID2D1RenderTargetAvailable(ID2D1RenderTarget* pRenderTarget)
{
  ZoneScoped;

  {
    auto* pTask     = mj::ThreadpoolCreateTask<mj::detail::LoadFolderIconTask>();
    pTask->pParent  = this;
    pTask->resource = IDB_FOLDER;
    mj::ThreadpoolSubmitTask(pTask);
  }
  {
    auto* pTask     = mj::ThreadpoolCreateTask<mj::detail::LoadFileIconTask>();
    pTask->pParent  = this;
    pTask->resource = IDB_DOCUMENT;
    mj::ThreadpoolSubmitTask(pTask);
  }

  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &this->pBlackBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFF0000), &this->pEntryHighlightBrush));
}

void mj::DirectoryNavigationPanel::OnIDWriteFactoryAvailable(IDWriteFactory* pFactory)
{
  ZoneScoped;
  MJ_ERR_HRESULT(pFactory->CreateTextFormat(L"Segoe UI",                    // Font name
                                            nullptr,                        // Font collection
                                            DWRITE_FONT_WEIGHT_NORMAL,      // Font weight
                                            DWRITE_FONT_STYLE_NORMAL,       // Font style
                                            DWRITE_FONT_STRETCH_NORMAL,     // Font stretch
                                            ::ConvertPointSizeToDIP(10.0f), // Font size
                                            L"",                            // Locale name
                                            &this->pTextFormat));
  this->CheckEverythingQueryPrerequisites();
}
