#pragma once

struct ID2D1Bitmap;
struct ID2D1SolidColorBrush;
struct ID2D1RenderTarget;

namespace mj
{
  class AllocatorBase;
}

namespace res
{
  namespace d2d1
  {
    class BitmapObserver
    {
    public:
      virtual void OnIconBitmapAvailable(ID2D1Bitmap* pBitmap, uint16_t resource) = 0;
    };
    void AddBitmapObserver(BitmapObserver* pObserver);
    void RemoveBitmapObserver(BitmapObserver* pObserver);

    void Init(mj::AllocatorBase* pAllocator);
    void Load(ID2D1RenderTarget* pRenderTarget);
    void Destroy();

    ID2D1SolidColorBrush* Brush();
    ID2D1Bitmap* FolderIcon();
    ID2D1Bitmap* FileIcon();
  } // namespace d2d1
} // namespace res
