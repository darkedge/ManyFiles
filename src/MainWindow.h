#include "TextEditor.h"
#include "mj_win32.h"

class MainWindow
{
public:
  static constexpr const auto kClassName = L"DirectWritePadDemo";
  static ATOM RegisterWindowClass();
  static LRESULT CALLBACK WindowProc(HWND parentHwnd, UINT message, WPARAM wParam, LPARAM lParam);

  HRESULT Initialize();
  WPARAM RunMessageLoop();

public:
  STDMETHODIMP CreateFontFromLOGFONT(const LOGFONT& logFont, OUT IDWriteFont** font);
  STDMETHODIMP GetFontFamilyName(IDWriteFont* font, OUT wchar_t* fontFamilyName, UINT32 fontFamilyNameLength);

private:
  HWND pHwnd = nullptr;
  mj::ComPtr<IDWriteFactory> dwriteFactory_;
  mj::ComPtr<ID2D1Factory> d2dFactory_;
  mj::ComPtr<RenderTarget> pRenderTarget;

  TextEditor* pTextEditor = nullptr;
  //BinaryView* pBinaryView = nullptr;

private:
  HRESULT CreateRenderTarget(HWND hwnd);
  HRESULT FormatSampleLayout(IDWriteTextLayout* textLayout);

  void OnSize();
  void OnCommand(UINT commandId);
  HRESULT OnChooseFont();

  void OpenFileDialog();
  void UpdateMenuToCaret();
  void RedrawTextEditor();
};
