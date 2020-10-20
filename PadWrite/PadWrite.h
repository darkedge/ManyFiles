#include "Common.h"
#include "mj_win32.h"

class MainWindow
{
public:
  MainWindow();

  static ATOM RegisterWindowClass();
  static LRESULT CALLBACK WindowProc(HWND parentHwnd, UINT message, WPARAM wParam, LPARAM lParam);

  HRESULT Initialize();
  WPARAM RunMessageLoop();

public:
  STDMETHODIMP CreateFontFromLOGFONT(const LOGFONT& logFont, OUT IDWriteFont** font);

  STDMETHODIMP GetFontFamilyName(IDWriteFont* font, OUT wchar_t* fontFamilyName, UINT32 fontFamilyNameLength);

protected:
  enum RenderTargetType
  {
    RenderTargetTypeD2D,
    RenderTargetTypeDW,
    RenderTargetTypeTotal
  };

  HWND pHwnd;
  mj::ComPtr<IDWriteFactory> dwriteFactory_;
  mj::ComPtr<ID2D1Factory> d2dFactory_;

  mj::ComPtr<RenderTarget> renderTarget_;
  RenderTargetType renderTargetType_;

  mj::ComPtr<TextEditor> pTextEditor;

protected:
  HRESULT CreateRenderTarget(HWND hwnd, RenderTargetType renderTargetType);
  HRESULT FormatSampleLayout(IDWriteTextLayout* textLayout);

  void OnSize();
  void OnCommand(UINT commandId);
  HRESULT OnChooseFont();

  void UpdateMenuToCaret();
  void RedrawTextEditor();
};
