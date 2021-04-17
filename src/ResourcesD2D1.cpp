#include "ResourcesD2D1.h"
#include "../tracy/Tracy.hpp"
#include "ErrorExit.h"
#include "../vs/ManyFiles/resource.h"
#include "Threadpool.h"
#include "ServiceLocator.h"
#include "stb_image.h"
#include <d2d1.h>

static ID2D1SolidColorBrush* pBlackBrush;
static ID2D1SolidColorBrush* pScrollbarForegroundBrush;
static ID2D1SolidColorBrush* pScrollbarBackgroundBrush;
static ID2D1SolidColorBrush* pEntryHoverBrush;
static ID2D1SolidColorBrush* pResizeControlBrush;
static ID2D1SolidColorBrush* pControlBackgroundBrush;
static ID2D1Bitmap* pFolderIcon;
static ID2D1Bitmap* pFileIcon;

// TODO: These should be sets, not arrays
static mj::ArrayList<res::d2d1::BitmapObserver*> s_BitmapObservers;

// TODO: Provide fallback if a resource is null?

void res::d2d1::Init(mj::AllocatorBase* pAllocator)
{
  s_BitmapObservers.Init(pAllocator);
}

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
      D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)), ppBitmap));
}

// Requires: D2D1RenderTarget
struct LoadIconBitmapTask : public mj::Task
{
  // In
  WORD resource = 0;

  // Out
  ID2D1Bitmap* pBitmap = nullptr;

  virtual void Execute() override
  {
    ZoneScoped;
    LoadBitmapFromResource(this->resource, &this->pBitmap);
  }

  virtual void OnDone() override
  {
    ZoneScoped;
    for (auto pObserver : s_BitmapObservers)
    {
      pObserver->OnIconBitmapAvailable(pBitmap, resource);
    }
  }

  virtual void Destroy() override
  {
    ZoneScoped;
    MJ_SAFE_RELEASE(this->pBitmap);
  }
};

static void LoadIconBitmap(WORD resource)
{
  auto* pTask     = mj::ThreadpoolCreateTask<LoadIconBitmapTask>();
  pTask->resource = resource;
  mj::ThreadpoolSubmitTask(pTask);
}

void res::d2d1::Load(ID2D1RenderTarget* pRenderTarget)
{
  ZoneScoped;

  MJ_EXIT_NULL(pRenderTarget);
  Destroy();

  LoadIconBitmap(IDB_FOLDER);
  LoadIconBitmap(IDB_DOCUMENT);

  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xC2C3C9), &pScrollbarForegroundBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xE8E8EC), &pScrollbarBackgroundBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xE5F3FF), &pEntryHoverBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xD4D4D4), &pResizeControlBrush));
  MJ_ERR_HRESULT(pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0xFFFFFF), &pControlBackgroundBrush));
}

void res::d2d1::Destroy()
{
  MJ_SAFE_RELEASE(pBlackBrush);
  MJ_SAFE_RELEASE(pScrollbarForegroundBrush);
  MJ_SAFE_RELEASE(pScrollbarBackgroundBrush);
  MJ_SAFE_RELEASE(pEntryHoverBrush);
  MJ_SAFE_RELEASE(pResizeControlBrush);
  MJ_SAFE_RELEASE(pControlBackgroundBrush);
  MJ_SAFE_RELEASE(pFolderIcon);
  MJ_SAFE_RELEASE(pFileIcon);
}

ID2D1SolidColorBrush* res::d2d1::BlackBrush()
{
  MJ_EXIT_NULL(pBlackBrush);
  return pBlackBrush;
}

ID2D1SolidColorBrush* res::d2d1::ScrollbarForegroundBrush()
{
  MJ_EXIT_NULL(pScrollbarForegroundBrush);
  return pScrollbarForegroundBrush;
}

ID2D1SolidColorBrush* res::d2d1::ScrollbarBackgroundBrush()
{
  MJ_EXIT_NULL(pScrollbarBackgroundBrush);
  return pScrollbarBackgroundBrush;
}

ID2D1SolidColorBrush* res::d2d1::EntryHoverBrush()
{
  MJ_EXIT_NULL(pEntryHoverBrush);
  return pEntryHoverBrush;
}

ID2D1SolidColorBrush* res::d2d1::ResizeControlBrush()
{
  MJ_EXIT_NULL(pResizeControlBrush);
  return pResizeControlBrush;
}

ID2D1SolidColorBrush* res::d2d1::ControlBackgroundBrush()
{
  MJ_EXIT_NULL(pControlBackgroundBrush);
  return pControlBackgroundBrush;
}

ID2D1Bitmap* res::d2d1::FolderIcon()
{
  return pFolderIcon;
}

ID2D1Bitmap* res::d2d1::FileIcon()
{
  return pFileIcon;
}

void res::d2d1::AddBitmapObserver(BitmapObserver* pObserver)
{
  s_BitmapObservers.Add(pObserver);
  // TODO: Pass all bitmaps? We don't keep a list...
}

void res::d2d1::RemoveBitmapObserver(BitmapObserver* pObserver)
{
  s_BitmapObservers.RemoveAll(pObserver);
}
