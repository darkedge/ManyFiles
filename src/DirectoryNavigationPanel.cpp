#include "DirectoryNavigationPanel.h"
#include "ErrorExit.h"
#include "ServiceLocator.h"
#include <wincodec.h>
#include "Everything.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include <shellapi.h>
#include "..\3rdparty\tracy\Tracy.hpp"
#include "Threadpool.h"
#include "../vs/resource.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT(x)

#include "ncrt_memory.h"

#define STBI_MALLOC(sz)        mj::malloc(sz)
#define STBI_REALLOC(p, newsz) mj::realloc(p, newsz)
#define STBI_FREE(p)           mj::free(p)

#pragma function(abs)
int abs(int a)
{
  return a > 0 ? a : -a;
}

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

  auto* pContext = svc::D2D1RenderTarget();
  MJ_UNINITIALIZED ID2D1Bitmap* pBitmap;
  MJ_ERR_HRESULT(pContext->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap));

  return pBitmap;
}

struct LoadFolderIconContext : public mj::Task
{
  HICON hIcon;
  mj::DirectoryNavigationPanel* pParent;

  virtual void Execute() override
  {
    ZoneScoped;
    this->hIcon = static_cast<HICON>(
        ::LoadImageW(HINST_THISCOMPONENT, MAKEINTRESOURCEW(IDI_FOLDER), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR));
  }

  virtual void OnDone() override
  {
    this->pParent->OnLoadFolderIcon(this->hIcon);
  }
};

struct EverythingQueryContext : public mj::Task
{
  mj::DirectoryNavigationPanel* pParent = nullptr;
  mj::String directory;
  mj::Allocation searchBuffer;

  EverythingQueryContext() : directory(nullptr, 0)
  {
  }

  virtual void Execute() override
  {
    ZoneScoped;
    mj::LinearAllocator alloc;
    alloc.Init(this->searchBuffer);

    mj::ArrayList<wchar_t> arrayList;
    arrayList.Init(&alloc, this->searchBuffer.numBytes / sizeof(wchar_t));

    mj::StringBuilder sb;
    sb.SetArrayList(&arrayList);

    mj::String search = sb.Append(L"\"")             //
                            .Append(this->directory) //
                            .Append(L"\" !\"")       //
                            .Append(this->directory) //
                            .Append(L"*\\*\"")       //
                            .ToString();

    Everything_SetSearchW(search.ptr);
    {
      static_cast<void>(Everything_QueryW(TRUE));
    }
  }

  virtual void OnDone() override
  {
    this->pParent->OnEverythingQuery();
  }
};

void mj::DirectoryNavigationPanel::Init(AllocatorBase* pAllocator)
{
  ZoneScoped;
  svc::AddWicFactoryObserver(this);
  svc::AddID2D1RenderTargetObserver(this);
  svc::AddIDWriteFactoryObserver(this);

  // Allocator setup
  this->pAllocator    = pAllocator;
  this->searchBuffer  = this->pAllocator->Allocation(1 * 1024);
  this->resultsBuffer = this->pAllocator->Allocation(1 * 1024 * 1024);
  MJ_EXIT_NULL(this->searchBuffer.pAddress);
  MJ_EXIT_NULL(this->resultsBuffer.pAddress);
  this->entries.Init(this->pAllocator);

  // Start tasks
  {
    LoadFolderIconContext* pTask = mj::ThreadpoolCreateTask<LoadFolderIconContext>();
    pTask->pParent               = this;
    mj::ThreadpoolSubmitTask(pTask);
  }

  {
    EverythingQueryContext* pTask = mj::ThreadpoolCreateTask<EverythingQueryContext>();
    pTask->pParent                = this;
    pTask->directory              = mj::String(LR"(C:\)");
    pTask->searchBuffer           = this->searchBuffer;
    this->queryDone               = false;
    mj::ThreadpoolSubmitTask(pTask);
  }
}

namespace mj
{
  static ID2D1Bitmap* WicLess()
  {
    ZoneScoped;

    // Resource management.
    HRSRC imageResHandle       = NULL;
    HGLOBAL imageResDataHandle = NULL;
    void* pImageFile           = NULL;

    // Locate the resource in the application's executable.
    // imageResHandle = FindResourceW(NULL, L"IDB_FOLDER", L"Image");
    imageResHandle = FindResourceW(NULL, MAKEINTRESOURCEW(IDB_FOLDER), L"PNG");

    HRESULT hr = (imageResHandle ? S_OK : E_FAIL);

    // Load the resource to the HGLOBAL.
    if (SUCCEEDED(hr))
    {
      ZoneScopedN("LoadResource");
      imageResDataHandle = LoadResource(NULL, imageResHandle);
      hr                 = (imageResDataHandle ? S_OK : E_FAIL);
    }

    auto size = ::SizeofResource(nullptr, imageResHandle);

    // Lock the resource to retrieve memory pointer.
    if (SUCCEEDED(hr))
    {
      ZoneScopedN("LockResource");
      pImageFile = LockResource(imageResDataHandle);
      hr         = (pImageFile ? S_OK : E_FAIL);
    }

    int numComponents = 4;
    int x, y, n;
    // ... process data if not NULL ...
    // ... x = width, y = height, n = # 8-bit components per pixel ...
    // ... replace '0' with '1'..'4' to force that many components per pixel
    // ... but 'n' will always be the number that it would have been if you said 0
    uint32_t* data = reinterpret_cast<uint32_t*>(
        stbi_load_from_memory(static_cast<stbi_uc*>(pImageFile), size, &x, &y, &n, numComponents));
    MJ_DEFER(stbi_image_free(data));

    auto* pContext = svc::D2D1RenderTarget();

    MJ_UNINITIALIZED ID2D1Bitmap* pBitmap;
    hr = pContext->CreateBitmap(
        D2D1::SizeU(x, y), data, x * numComponents,
        D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), &pBitmap);

    return pBitmap;
  }

} // namespace mj

void mj::DirectoryNavigationPanel::CheckFolderIconPrerequisites()
{
  ZoneScoped;
  if (this->pFolderIconHandle && svc::WicFactory() && svc::D2D1RenderTarget())
  {
    this->pFolderIcon = WicLess();
    static_cast<void>(::DestroyIcon(this->pFolderIconHandle));
    CheckEverythingQueryPrerequisites();
  }
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
    Entry* pEntries = this->entries.Reserve(numResults);

    wchar_t fullPathName[MAX_PATH];
    for (DWORD i = 0; i < numResults; i++)
    {
      auto& entry = pEntries[i];

      String string(Everything_GetResultFileNameW(i));
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
          MJ_ERR_ZERO(SHGetFileInfoW(fullPathName, 0, &fileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON));
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

void mj::DirectoryNavigationPanel::Paint()
{
  ZoneScoped;
  auto* pContext = svc::D2D1RenderTarget();
  if (pContext)
  {
    auto point = D2D1::Point2F(16.0f, 0.0f);
    for (const auto& entry : this->entries)
    {
      MJ_UNINITIALIZED DWRITE_LINE_METRICS metrics;
      MJ_UNINITIALIZED UINT32 count;
      MJ_ERR_HRESULT(entry.pTextLayout->GetLineMetrics(&metrics, 1, &count));
      pContext->DrawTextLayout(point, entry.pTextLayout, this->pBlackBrush);
      auto iconSize = entry.pIcon->GetPixelSize();
      float width   = static_cast<float>(iconSize.width);
      float height  = static_cast<float>(iconSize.height);
      pContext->DrawBitmap(entry.pIcon, D2D1::RectF(0.0f, point.y, width, point.y + height));

      // Always draw images on integer coordinates
      point.y += static_cast<int>(metrics.height);
    }
  }
}

void mj::DirectoryNavigationPanel::Destroy()
{
  ZoneScoped;
  this->pAllocator->Free(this->searchBuffer.pAddress);
  this->pAllocator->Free(this->resultsBuffer.pAddress);

  for (auto& entry : this->entries)
  {
    if (entry.pIcon != this->pFolderIcon)
    {
      entry.pIcon->Release();
    }
    entry.pTextLayout->Release();
  }
  this->entries.Clear();

  if (this->pBlackBrush)
    this->pBlackBrush->Release();

  if (this->pFolderIcon)
    this->pFolderIcon->Release();

  svc::RemoveWicFactoryObserver(this);
  svc::RemoveID2D1RenderTargetObserver(this);
  svc::RemoveIDWriteFactoryObserver(this);
}

void mj::DirectoryNavigationPanel::OnLoadFolderIcon(HICON hIcon)
{
  this->pFolderIconHandle = hIcon;
  this->CheckFolderIconPrerequisites();
}

void mj::DirectoryNavigationPanel::OnEverythingQuery()
{
  queryDone = true;
  this->CheckEverythingQueryPrerequisites();
}

void mj::DirectoryNavigationPanel::OnWICFactoryAvailable(IWICImagingFactory* pFactory)
{
  this->CheckFolderIconPrerequisites();
}

void mj::DirectoryNavigationPanel::OnID2D1RenderTargetAvailable(ID2D1RenderTarget* pContext)
{
  ZoneScoped;
  this->CheckFolderIconPrerequisites();
  MJ_ERR_HRESULT(pContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush));
}

void mj::DirectoryNavigationPanel::OnIDWriteFactoryAvailable(IDWriteFactory* pFactory)
{
  ZoneScoped;
  MJ_ERR_HRESULT(pFactory->CreateTextFormat(L"Segoe UI",                  // Font name
                                            nullptr,                      // Font collection
                                            DWRITE_FONT_WEIGHT_NORMAL,    // Font weight
                                            DWRITE_FONT_STYLE_NORMAL,     // Font style
                                            DWRITE_FONT_STRETCH_NORMAL,   // Font stretch
                                            ConvertPointSizeToDIP(10.0f), // Font size
                                            L"",                          // Locale name
                                            &this->pTextFormat));
  this->CheckEverythingQueryPrerequisites();
}
