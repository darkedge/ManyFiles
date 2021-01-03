#include "DirectoryNavigationPanel.h"
#include "ErrorExit.h"
#include <d2d1_1.h>
#include <dwrite.h>
#include "Everything.h"
#include "ServiceLocator.h"
#include <wincodec.h>

static float ConvertPointSizeToDIP(float points)
{
  return ((points / 72.0f) * 96.0f);
}

ID2D1Bitmap1* mj::DirectoryNavigationPanel::ConvertIcon(HICON hIcon)
{
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

  auto* pContext = svc::D2D1DeviceContext();
  MJ_UNINITIALIZED ID2D1Bitmap1* pBitmap;
  MJ_ERR_HRESULT(pContext->CreateBitmapFromWicBitmap(pConverter, nullptr, &pBitmap));

  return pBitmap;
}

void mj::DirectoryNavigationPanel::LoadIcons()
{
  MJ_UNINITIALIZED SHSTOCKICONINFO info;
  info.cbSize = sizeof(SHSTOCKICONINFO);
  MJ_ERR_HRESULT(::SHGetStockIconInfo(SIID_FOLDER, SHGSI_ICON | SHGSI_SMALLICON, &info));
  MJ_DEFER(::DestroyIcon(info.hIcon));
  this->pFolderIcon = this->ConvertIcon(info.hIcon);
}

void mj::DirectoryNavigationPanel::Init(AllocatorBase* pAllocator)
{
  this->LoadIcons();
  auto* pContext = svc::D2D1DeviceContext();
  auto* pFactory = svc::DWriteFactory();

  MJ_ERR_HRESULT(pContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBlackBrush));
  MJ_ERR_HRESULT(pFactory->CreateTextFormat(L"Segoe UI",                  // Font name
                                            nullptr,                      // Font collection
                                            DWRITE_FONT_WEIGHT_NORMAL,    // Font weight
                                            DWRITE_FONT_STYLE_NORMAL,     // Font style
                                            DWRITE_FONT_STRETCH_NORMAL,   // Font stretch
                                            ConvertPointSizeToDIP(10.0f), // Font size
                                            L"",                          // Locale name
                                            &this->pTextFormat));

  this->pAllocator    = pAllocator;
  this->searchBuffer  = this->pAllocator->Allocation(1 * 1024);
  this->resultsBuffer = this->pAllocator->Allocation(1 * 1024 * 1024);
  MJ_EXIT_NULL(this->searchBuffer.pAddress);
  MJ_EXIT_NULL(this->resultsBuffer.pAddress);
  this->entries.Init(this->pAllocator);

  this->Navigate(String(LR"(C:\)"));
}

void mj::DirectoryNavigationPanel::Navigate(const mj::String& directory)
{
  LinearAllocator alloc;
  alloc.Init(this->searchBuffer);

  ArrayList<wchar_t> arrayList;
  arrayList.Init(&alloc, this->searchBuffer.numBytes / sizeof(wchar_t));

  StringBuilder sb;
  sb.SetArrayList(&arrayList);

  String search = sb.Append(L"\"")       //
                      .Append(directory) //
                      .Append(L"\" !\"") //
                      .Append(directory) //
                      .Append(L"*\\*\"") //
                      .ToString();

  Everything_SetSearchW(search.ptr);
  static_cast<void>(Everything_QueryW(TRUE));

  // Display results.
  DWORD numResults = Everything_GetNumResults();
  this->entries.Clear();
  Entry* pEntries = this->entries.Reserve(numResults);
  auto* pFactory  = svc::DWriteFactory();
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
      MJ_ERR_ZERO(SHGetFileInfoW(fullPathName, 0, &fileInfo, sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_SMALLICON));
      MJ_DEFER(::DestroyIcon(fileInfo.hIcon));
      entry.pIcon = this->ConvertIcon(fileInfo.hIcon);
    }
    else if (Everything_IsFolderResult(i))
    {
      entry.pIcon = this->pFolderIcon;
    }
  }
}

void mj::DirectoryNavigationPanel::Paint()
{
  auto* pContext = svc::D2D1DeviceContext();

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

void mj::DirectoryNavigationPanel::Destroy()
{
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
}
