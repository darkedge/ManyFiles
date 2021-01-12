#include "MainWindow.h"
#include "ErrorExit.h"
#include "ServiceProvider.h"
#include "DirectoryNavigationPanel.h"
#include <dwrite.h>
#include <wincodec.h>
#include "../3rdparty/tracy/Tracy.hpp"
#include "Threadpool.h"

#define MJ_WM_SIZE (WM_USER + 0)
#define MJ_WM_TASK (WM_USER + 1)

struct CreateIWICImagingFactoryContext : public mj::Task
{
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;
  MJ_UNINITIALIZED IWICImagingFactory* pWicFactory;

  virtual void Execute() override
  {
    ZoneScoped;

    {
      ZoneScopedN("CoInitializeEx");
      MJ_ERR_HRESULT(::CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE));
    }

    {
      ZoneScopedN("CoCreateInstance");
      // TODO: Can cause exception c0000374?
      MJ_ERR_HRESULT(::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory,
                                        (IID_PPV_ARGS(&this->pWicFactory))));
    }
  }

  virtual void OnDone() override
  {
    svc::ProvideWicFactory(this->pWicFactory);
  }
};

struct CreateID2D1RenderTargetContext : public mj::Task
{
  // In
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;

  // Out
  MJ_UNINITIALIZED ID2D1DCRenderTarget* pRenderTarget;

  virtual void Execute() override
  {
    ZoneScoped;
    // Create a DC render target.
    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_SOFTWARE, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0, 0,
        D2D1_RENDER_TARGET_USAGE_NONE, D2D1_FEATURE_LEVEL_DEFAULT);

    MJ_UNINITIALIZED ID2D1Factory* pFactory;
    MJ_ERR_HRESULT(::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory));
    MJ_ERR_HRESULT(pFactory->CreateDCRenderTarget(&props, &this->pRenderTarget));
    pFactory->Release();
  }

  virtual void OnDone() override
  {
    this->pMainWindow->OnCreateID2D1RenderTarget(this->pRenderTarget);
  }
};

struct CreateDWriteFactoryTask : public mj::Task
{
  // In
  MJ_UNINITIALIZED mj::MainWindow* pMainWindow;

  // Out
  MJ_UNINITIALIZED IDWriteFactory* pDWriteFactory;

  virtual void Execute() override
  {
    ZoneScoped;

    MJ_ERR_HRESULT(::DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                         reinterpret_cast<IUnknown**>(&pDWriteFactory)));
  }

  virtual void OnDone() override
  {
    svc::ProvideDWriteFactory(pDWriteFactory);
    pDWriteFactory->Release();
  }
};

void mj::MainWindow::OnCreateID2D1RenderTarget(ID2D1DCRenderTarget* pRenderTarget)
{
  this->pRenderTarget = pRenderTarget;
  svc::ProvideD2D1RenderTarget(pRenderTarget);
}

void mj::MainWindow::Init()
{
  ZoneScoped;

  auto pAllocator = svc::GeneralPurposeAllocator();

  svc::Init(svc::GeneralPurposeAllocator());

  this->pDirectoryNavigationPanel = pAllocator->New<DirectoryNavigationPanel>();
  this->pDirectoryNavigationPanel->Init(pAllocator);

  // Start a bunch of tasks

  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateIWICImagingFactoryContext>();
    pTask->pMainWindow = this;
    mj::ThreadpoolSubmitTask(pTask);
  }
  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateID2D1RenderTargetContext>();
    pTask->pMainWindow = this;
    mj::ThreadpoolSubmitTask(pTask);
  }
  {
    auto pTask         = mj::ThreadpoolCreateTask<CreateDWriteFactoryTask>();
    pTask->pMainWindow = this;
    mj::ThreadpoolSubmitTask(pTask);
  }

  // TODO: Put this somewhere else
  {
    ZoneScopedN("GetLogicalDriveStringsW");
    DWORD dw      = ::GetLogicalDriveStringsW(0, nullptr);
    wchar_t* pBuf = pAllocator->New<wchar_t>(dw);
    MJ_DEFER(pAllocator->Free(pBuf));
    dw           = ::GetLogicalDriveStringsW(dw, pBuf);
    wchar_t* ptr = pBuf;
    String str(nullptr);
    while (ptr < pBuf + dw)
    {
      if (*ptr != 0)
      {
        wchar_t* pBegin = ptr;
        while (*ptr != 0)
        {
          ptr++;
        }
        str = String(pBegin, ptr - pBegin);
      }
      ptr++;
    }
  }
}

void mj::MainWindow::Resize()
{
  ZoneScoped;
  if (pRenderTarget)
  {
    MJ_UNINITIALIZED RECT clientArea;
    ::GetClientRect(svc::MainWindowHandle(), &clientArea);
    MJ_ERR_HRESULT(pRenderTarget->BindDC(GetDC(svc::MainWindowHandle()), &clientArea));
  }
}

void mj::MainWindow::Paint()
{
  ZoneScoped;
  if (this->pRenderTarget)
  {
    {
      ZoneScopedN("BeginDraw");
      this->pRenderTarget->BeginDraw();
    }

    {
      ZoneScopedN("Clear");
      this->pRenderTarget->Clear(D2D1::ColorF(1.0f, 1.0f, 1.0f));
    }
    this->pDirectoryNavigationPanel->Paint();

    {
      ZoneScopedN("EndDraw");
      MJ_ERR_HRESULT(this->pRenderTarget->EndDraw());
    }
  }
}

void mj::MainWindow::Destroy()
{
  ZoneScoped;
  if (this->pDirectoryNavigationPanel)
  {
    this->pDirectoryNavigationPanel->Destroy();
    this->pDirectoryNavigationPanel = nullptr;
  }

  if (this->pRenderTarget)
  {
    svc::ProvideD2D1RenderTarget(nullptr);
    static_cast<void>(pRenderTarget->Release());
    this->pRenderTarget = nullptr;
  }

  ::CoUninitialize();
}

/// <summary>
/// Main WindowProc function
/// </summary>
LRESULT CALLBACK mj::MainWindow::WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  mj::MainWindow* pMainWindow = reinterpret_cast<mj::MainWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    ZoneScopedN("WM_NCCREATE");
    // Loaded DLLs: uxtheme, combase, msctf, oleaut32

    // Copy the lpParam from CreateWindowEx to this window's user data
    CREATESTRUCT* pcs = reinterpret_cast<CREATESTRUCT*>(lParam);
    pMainWindow       = reinterpret_cast<mj::MainWindow*>(pcs->lpCreateParams);
    MJ_ERR_ZERO_VALID(::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pMainWindow)));
    svc::ProvideMainWindowHandle(hWnd);

    mj::ThreadpoolSetWindowHandle(hWnd);
    pMainWindow->Init();

    return ::DefWindowProcW(hWnd, message, wParam, lParam);
  }
  case WM_SIZE:
    // Do not resize while dragging the window border
    pMainWindow->s_Resize = true;
    return 0;
  case MJ_WM_SIZE:
  {
    pMainWindow->Resize();
    MJ_ERR_ZERO(::InvalidateRect(hWnd, nullptr, FALSE));
    return 0;
  }
  case WM_DESTROY:
    mj::ThreadpoolDestroy();
    ::PostQuitMessage(0);
    return 0;
  case WM_PAINT:
  {
    MJ_UNINITIALIZED PAINTSTRUCT ps;
    static_cast<void>(::BeginPaint(hWnd, &ps));
    pMainWindow->Paint();
    static_cast<void>(::EndPaint(hWnd, &ps));
    return 0;
  }
  case MJ_WM_TASK:
  {
    mj::ThreadpoolTaskEnd(wParam);
    return 0;
  }
  default:
    break;
  }

  return ::DefWindowProcW(hWnd, message, wParam, lParam);
}

template <typename T>
static mj::AllocatorBase* CreateAllocator()
{
  static_assert(std::is_base_of<mj::AllocatorBase, T>::value);

#ifdef TRACY_ENABLE

#endif
}

void mj::MainWindow::Run()
{
  MJ_DEFER(this->Destroy());
  mj::ThreadpoolInit(MJ_WM_TASK);

  mj::HeapAllocator generalPurposeAllocator;
  generalPurposeAllocator.Init();
  MJ_UNINITIALIZED mj::AllocatorBase* pAllocator;
  pAllocator = &generalPurposeAllocator;
  svc::ProvideGeneralPurposeAllocator(pAllocator);

  MJ_UNINITIALIZED ATOM cls;
  WNDCLASSEXW wc   = {};
  wc.cbSize        = sizeof(WNDCLASSEX);
  wc.lpfnWndProc   = mj::MainWindow::WindowProc;
  wc.hInstance     = HINST_THISCOMPONENT;
  wc.lpszClassName = L"Class Name";
  wc.hCursor       = ::LoadCursorW(nullptr, IDC_ARROW);
  MJ_ERR_ZERO(cls = ::RegisterClassExW(&wc));
  LPWSTR classAtom = MAKEINTATOM(cls);

  MJ_UNINITIALIZED HWND hWnd;
  {
    ZoneScopedN("CreateWindowExW");
    hWnd = ::CreateWindowExW(0,                                       // Optional window styles.
                             classAtom,                               // Window class
                             L"Window Title",                         // Window text
                             WS_OVERLAPPEDWINDOW | WS_VISIBLE,        // Window style
                             CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720, // Size and pCurrent
                             nullptr,                                 // Parent window
                             nullptr,                                 // Menu
                             HINST_THISCOMPONENT,                     // Instance handle
                             this);                                   // Additional application data
  }

  // Run the message loop.
  MJ_UNINITIALIZED MSG msg;
  while (::GetMessageW(&msg, nullptr, 0, 0))
  {
    // If the message is translated, the return value is nonzero.
    // If the message is not translated, the return value is zero.
    static_cast<void>(::TranslateMessage(&msg));

    // The return value specifies the value returned by the window procedure.
    // Although its meaning depends on the message being dispatched,
    // the return value generally is ignored.
    static_cast<void>(::DispatchMessageW(&msg));

    if (s_Resize)
    {
      s_Resize = false;
      MJ_ERR_ZERO(::PostMessageW(hWnd, MJ_WM_SIZE, 0, 0));
    }
  }
}
