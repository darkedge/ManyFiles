#include "mj_textedit.h"
#include <d2d1.h>
#include "mj_common.h"
#include "mj_win32.h"
#include "loremipsum.h"
#include "stretchy_buffer.h"
#include "render_target_resources.h"
#include <Strsafe.h>

static constexpr size_t BUFFER_SIZE   = 2 * 1024 * 1024; // 2 MiB
static constexpr FLOAT SCROLLBAR_SIZE = 20.0f;

HRESULT mj::TextEdit::Init(HWND pParent, FLOAT margin, FLOAT parentWidth, FLOAT parentHeight)
{
  HRESULT hr = S_OK;

  this->widgetRect.left = margin;
  this->widgetRect.top  = margin;
  this->margin          = margin;
  this->Resize(parentWidth, parentHeight);

  // Init memory for buffer
  this->pMemory = VirtualAlloc(0,                        //
                               BUFFER_SIZE,              //
                               MEM_COMMIT | MEM_RESERVE, //
                               PAGE_READWRITE);

  if (!this->pMemory)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
  }

  if (SUCCEEDED(hr))
  {
    this->buf.Init(this->pMemory, ((char*)this->pMemory) + BUFFER_SIZE);
    this->buf.SetText(pLoremIpsum);
    this->text.Init();

    // CreateWindowExW will return null here, we must instead store it when handling
    // the WM_NCCREATE event (this is done before before CreateWindowExW returns)
    MJ_DISCARD(CreateWindowExW(WS_EX_STATICEDGE,                         //
                               L"TextEdit",                              //
                               L"",                                      //
                               WS_CHILDWINDOW | WS_VSCROLL | WS_VISIBLE, //
                               CW_USEDEFAULT,                            //
                               CW_USEDEFAULT,                            //
                               CW_USEDEFAULT,                            //
                               CW_USEDEFAULT,                            //
                               pParent,                                  //
                               NULL,                                     //
                               HINST_THISCOMPONENT,                      //
                               this));
    if (!this->hwnd)
    {
      hr = HRESULT_FROM_WIN32(GetLastError());
    }
  }

  return hr;
}

void mj::TextEdit::Resize(FLOAT width, FLOAT height)
{
  this->widgetRect.right  = this->widgetRect.left + width - 2.0f * this->margin;
  this->widgetRect.bottom = this->widgetRect.top + height - 2.0f * this->margin;
}

void mj::TextEdit::Destroy()
{
  if (this->pMemory)
  {
    MJ_DISCARD(VirtualFree(this->pMemory, 0, MEM_RELEASE));
    this->pMemory = nullptr;
  }
}

void mj::TextEdit::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  (void)lParam;
  (void)hwnd;
  switch (message)
  {
  case WM_CHAR:
    switch (wParam)
    {
    case 0x08: // Backspace
      this->buf.BackspaceAtCaret();
      break;
    case 0x0A: // Line feed
      break;
    case 0x1B: // Escape
      break;
    default:
      this->buf.InsertCharacterAtCaret((wchar_t)wParam);
      break;
    }
    break;
  case WM_KEYDOWN: // Navigation
    switch (wParam)
    {
    case VK_HOME:
      MJ_DISCARD(this->buf.JumpStartOfLine());
      break;
    case VK_END:
      MJ_DISCARD(this->buf.JumpEndOfLine());
      break;
    case VK_LEFT:
      MJ_DISCARD(this->buf.DecrementCaret());
      break;
    case VK_RIGHT:
      MJ_DISCARD(this->buf.IncrementCaret());
      break;
    case VK_UP:
      MJ_DISCARD(this->buf.CaretLinePrev());
      break;
    case VK_DOWN:
      MJ_DISCARD(this->buf.CaretLineNext());
      break;
    case VK_DELETE:
      this->buf.DeleteAtCaret();
      break;
    }
    break;
  }
}

#if 0
[[nodiscard]] static D2D1_POINT_2F operator*(const D2D1_MATRIX_3X2_F& matrix, const D2D1_POINT_2F& point)
{
  return D2D1::Matrix3x2F::ReinterpretBaseType(&matrix)->TransformPoint(point);
}
#endif

#if 0
[[nodiscard]] static RECT ToRect(const D2D_RECT_F& rectf)
{
  return RECT{
    (LONG)rectf.left,
    (LONG)rectf.top,
    (LONG)rectf.right,
    (LONG)rectf.bottom,
  };
}
#endif

void mj::TextEdit::Draw(mj::ComPtr<ID2D1HwndRenderTarget> pRenderTarget, RenderTargetResources* pResources)
{
  // Background
  pRenderTarget->FillRectangle(MJ_REF this->widgetRect, pResources->pTextEditBackgroundBrush.Get());
  pRenderTarget->PushAxisAlignedClip(MJ_REF this->widgetRect, D2D1_ANTIALIAS_MODE_ALIASED);
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F xBackGround;
  pRenderTarget->GetTransform(&xBackGround);

  pRenderTarget->SetTransform(xBackGround * D2D1::Matrix3x2F::Translation(this->widgetRect.left, this->widgetRect.top));
  MJ_UNINITIALIZED D2D1_MATRIX_3X2_F xWidget;
  pRenderTarget->GetTransform(&xWidget);

  pRenderTarget->SetTransform(xWidget * D2D1::Matrix3x2F::Translation(-this->scrollAmount.x, -this->scrollAmount.y));
  this->text.Draw(pRenderTarget, pResources, this->buf.GetVirtualCaretPosition());

  pRenderTarget->SetTransform(MJ_REF xWidget);

  pRenderTarget->PopAxisAlignedClip();
  pRenderTarget->SetTransform(MJ_REF xBackGround);
}

#if 0
[[nodiscard]] static bool RectContainsPoint(D2D1_RECT_F* pRect, D2D1_POINT_2F* pPoint)
{
  return ((pPoint->x >= pRect->left) && (pPoint->x <= pRect->right) && (pPoint->y >= pRect->top) &&
          (pPoint->y < (pRect->bottom)));
}
#endif

void mj::TextEdit::MouseDown(SHORT x, SHORT y)
{
  // Translate
  x -= (SHORT)this->widgetRect.left;
  y -= (SHORT)this->widgetRect.top;

  // Scroll bar
  this->drag.draggable = EDraggable::NONE;

  // Caret
  MJ_UNINITIALIZED UINT32 textPosition;
  if (this->text.GetTextPosition(x, y, MJ_REF textPosition))
  {
    this->buf.SetCaretPosition(textPosition);
  }
}

void mj::TextEdit::MouseUp()
{
  this->drag.draggable = mj::EDraggable::NONE;
}

void mj::TextEdit::MouseMove(SHORT x, SHORT y)
{
  // Translate
  SHORT xRel = x - (SHORT)this->widgetRect.left;
  SHORT yRel = y - (SHORT)this->widgetRect.top;

  // Check dragging
  switch (this->drag.draggable)
  {
  case EDraggable::HOR_SCROLLBAR:
  {
    const auto widgetWidth = (this->widgetRect.right - this->widgetRect.left);
    SHORT dx               = xRel - this->drag.mouseStartX;
    this->scrollAmount.x   = this->drag.start + (dx / widgetWidth * this->width);
    if (this->scrollAmount.x < 0.0f)
    {
      this->scrollAmount.x = 0.0f;
    }
    else if ((this->scrollAmount.x + widgetWidth) > this->width)
    {
      this->scrollAmount.x = (this->width - widgetWidth);
    }
  }
  break;
  default:
    break;
  }

  this->text.MouseMove(x, y);
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  // Relays messages for the text editor to the internal class.

  mj::TextEdit* pTextEdit = (mj::TextEdit*)(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

  switch (message)
  {
  case WM_NCCREATE:
  {
    // pTextEdit is not valid yet. Extract it from this message
    // and assign it to GWLP_USERDATA.
    CREATESTRUCT* pcs = (CREATESTRUCT*)(lParam);
    pTextEdit         = (mj::TextEdit*)(pcs->lpCreateParams);
    pTextEdit->SetWindowHandle(hwnd);
    // window->AddRef(); // implicit reference via HWND
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)pTextEdit);

    return DefWindowProcW(hwnd, message, wParam, lParam);
  }

#if 0
  case WM_PAINT:
  case WM_DISPLAYCHANGE:
    window->OnDraw();
    break;

  case WM_ERASEBKGND: // don't want flicker
    return true;

  case WM_DESTROY:
    window->OnDestroy();
    break;

  case WM_NCDESTROY:
    // Remove implicit reference via HWND.
    // After this, the window and data structure no longer exist.
    window->Release();
    break;

  case WM_KEYDOWN:
    window->OnKeyPress(static_cast<UINT>(wParam));
    break;

  case WM_CHAR:
    window->OnKeyCharacter(static_cast<UINT>(wParam));
    break;

  case WM_LBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
  case WM_MBUTTONDBLCLK:
  case WM_RBUTTONDBLCLK:
    SetFocus(hwnd);
    SetCapture(hwnd);
    window->OnMousePress(message, float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)));
    break;

  case WM_MOUSELEAVE:
  case WM_CAPTURECHANGED:
    window->OnMouseExit();
    break;

  case WM_LBUTTONUP:
  case WM_RBUTTONUP:
  case WM_MBUTTONUP:
    ReleaseCapture();
    window->OnMouseRelease(message, float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)));
    break;

  case WM_SETFOCUS:
  {
    RectF rect;
    window->GetCaretRect(rect);
    window->UpdateSystemCaret(rect);
  }
  break;

  case WM_KILLFOCUS:
    DestroyCaret();
    break;

  case WM_MOUSEMOVE:
    window->OnMouseMove(float(GET_X_LPARAM(lParam)), float(GET_Y_LPARAM(lParam)));
    break;

  case WM_MOUSEWHEEL:
  case WM_MOUSEHWHEEL:
  {
    // Retrieve the lines-to-scroll or characters-to-scroll user setting,
    // using a default value if the API failed.
    UINT userSetting;
    BOOL success = SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &userSetting, 0);
    if (success == FALSE)
      userSetting = 1;

    // Set x,y scroll difference,
    // depending on whether horizontal or vertical scroll.
    float zDelta  = GET_WHEEL_DELTA_WPARAM(wParam);
    float yScroll = (zDelta / WHEEL_DELTA) * userSetting;
    float xScroll = 0;
    if (message == WM_MOUSEHWHEEL)
    {
      xScroll = -yScroll;
      yScroll = 0;
    }

    window->OnMouseScroll(xScroll, yScroll);
  }
  break;

  case WM_VSCROLL:
  case WM_HSCROLL:
    window->OnScroll(message, LOWORD(wParam));
    break;

  case WM_SIZE:
  {
    UINT width  = LOWORD(lParam);
    UINT height = HIWORD(lParam);
    window->OnSize(width, height);
  }
  break;
#endif

  default:
    return DefWindowProc(hwnd, message, wParam, lParam);
  }
}

HRESULT mj::TextEdit::RegisterWindowClass()
{
  HRESULT hr = S_OK;

  WNDCLASSEX wcex;
  wcex.cbSize        = sizeof(WNDCLASSEX);
  wcex.style         = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
  wcex.lpfnWndProc   = &WindowProc;
  wcex.cbClsExtra    = 0;
  wcex.cbWndExtra    = sizeof(LONG_PTR);
  wcex.hInstance     = HINST_THISCOMPONENT;
  wcex.hIcon         = nullptr;
  wcex.hCursor       = LoadCursorW(nullptr, IDC_IBEAM);
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName  = nullptr;
  wcex.lpszClassName = L"TextEdit";
  wcex.hIconSm       = nullptr;

  ATOM atom = RegisterClassExW(&wcex);
  if (!atom)
  {
    hr = HRESULT_FROM_WIN32(GetLastError());
  }

  return hr;
}

HRESULT mj::TextEdit::CreateDeviceResources(mj::ComPtr<IDWriteFactory> pFactory,
                                            mj::ComPtr<IDWriteTextFormat> pTextFormat, FLOAT width, FLOAT height)
{
  // Set longest line width equal to widget width
  this->width = (this->widgetRect.right - this->widgetRect.left);

  HRESULT hr = this->text.CreateDeviceResources(pFactory, pTextFormat, this->buf, width, height);

  if (hr == S_OK)
  {
    FLOAT lineWidth = this->text.GetWidth();
    if (lineWidth >= this->width)
    {
      this->width = lineWidth;
    }
  }

  return hr;
}
