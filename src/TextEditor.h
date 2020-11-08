#pragma once
#define WIN32_LEAN_AND_LEAN
#define NOMINMAX
#include <Windows.h>
#include <dwrite.h>
#include "RenderTarget.h"
#include "EditableLayout.h"
#include "../3rdparty/tracy/Tracy.hpp"

class TextEditor
{
public:
  struct ESetSelectionMode
  {
    enum Enum
    {
      Left,             // cluster left
      Right,            // cluster right
      Up,               // line up
      Down,             // line down
      LeftChar,         // single character left (backspace uses it)
      RightChar,        // single character right
      LeftWord,         // single word left
      RightWord,        // single word right
      Home,             // front of line
      End,              // back of line
      First,            // very first position
      Last,             // very last position
      AbsoluteLeading,  // explicit position (for mouse click)
      AbsoluteTrailing, // explicit position, trailing edge
      All               // select all text
    };
  };

public:
  static constexpr const auto* kClassName = L"DirectWriteEdit";
  HRESULT static Create(HWND parentHwnd, const wchar_t* text, IDWriteTextFormat* textFormat, IDWriteFactory* factory,
                        TextEditor** textEditor);

  static ATOM RegisterWindowClass();
  static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
  HWND GetHwnd()
  {
    return hwnd_;
  }

  // Input dispatch
  void OnMousePress(UINT message, float x, float y);
  void OnMouseRelease(UINT message, float x, float y);
  void OnMouseMove(float x, float y);
  void OnMouseScroll(float xScroll, float yScroll);
  void OnMouseExit();
  void OnKeyPress(UINT32 keyCode);
  void OnKeyCharacter(UINT32 charCode);
  void MirrorXCoordinate(float& x);

  // Drawing/view change
  void OnDraw();
  void DrawPage(RenderTarget& target);
  void OnSize(UINT width, UINT height);
  void OnScroll(UINT message, UINT request);
  void GetCaretRect(D2D1_RECT_F& rect);
  void UpdateSystemCaret(const D2D1_RECT_F& rect);
  void SetRenderTarget(RenderTarget* target);
  void PostRedraw()
  {
    ZoneScoped;
    ::InvalidateRect(hwnd_, nullptr, FALSE);
  }

  // Used by the main application to respond to button commands.
  void CopyToClipboard();
  void DeleteSelection();
  void PasteFromClipboard();
  HRESULT InsertText(const wchar_t* text);
  HRESULT SetText(const wchar_t* text);

  // Layout/editing/caret navigation
  void UpdateCaretFormatting();
  bool SetSelection(ESetSelectionMode::Enum moveMode, UINT32 advance, bool extendSelection,
                    bool updateCaretFormat = true);
  DWRITE_TEXT_RANGE GetSelectionRange();
  UINT32 GetCaretPosition();
  IDWriteTextLayout* GetLayout()
  {
    return pTextLayout.Get();
  }
  EditableLayout::CaretFormat& GetCaretFormat()
  {
    return caretFormat_;
  }

  // Current view
  float GetAngle()
  {
    return angle_;
  }
  float SetAngle(float angle, bool relativeAdjustement);
  void SetScale(float scaleX, float scaleY, bool relativeAdjustement);
  void GetScale(float* scaleX, float* scaleY);
  void GetViewMatrix(DWRITE_MATRIX* matrix) const;
  void GetInverseViewMatrix(DWRITE_MATRIX* matrix) const;
  void ResetView();
  void RefreshView();

private:
  HRESULT Initialize(HWND parentHwnd, const wchar_t* text, IDWriteTextFormat* textFormat);
  void InitDefaults();
  void InitViewDefaults();

  bool SetSelectionFromPoint(float x, float y, bool extendSelection);
  void AlignCaretToNearestCluster(bool isTrailingHit = false, bool skipZeroWidth = false);

  void UpdateScrollInfo();
  void ConstrainViewOrigin();

  void GetLineMetrics(mj::ArrayList<DWRITE_LINE_METRICS>& lineMetrics);
  void GetLineFromPosition(const DWRITE_LINE_METRICS* lineMetrics, // [lineCount]
                           UINT32 lineCount, UINT32 textPosition, UINT32* lineOut, UINT32* linePositionOut);

private:
  // Creation/destruction
  TextEditor(IDWriteFactory* factory);

  HWND hwnd_ = nullptr;

  RenderTarget* renderTarget_ = nullptr;            // Not owned
  mj::ComPtr<DrawingEffect> pageBackgroundEffect_;  // Owned
  mj::ComPtr<DrawingEffect> textSelectionEffect_;   // Owned
  mj::ComPtr<DrawingEffect> imageSelectionEffect_;  // Owned
  mj::ComPtr<DrawingEffect> caretBackgroundEffect_; // Owned
  mj::ComPtr<IDWriteTextLayout> pTextLayout;        // Owned
  EditableLayout layoutEditor_;                     // Owned

  mj::Allocator allocator;
  mj::ArrayList<wchar_t> text_;

  // Selection/Caret navigation
  ///
  // caretAnchor equals caretPosition when there is no selection.
  // Otherwise, the anchor holds the point where shift was held
  // or left drag started.
  //
  // The offset is used as a sort of trailing edge offset from
  // the caret position. For example, placing the caret on the
  // trailing side of a surrogate pair at the beginning of the
  // text would place the position at zero and offset of two.
  // So to get the absolute leading position, sum the two.
  UINT32 caretAnchor_;
  UINT32 caretPosition_;
  UINT32 caretPositionOffset_; // > 0 used for trailing edge

  // Current attributes of the caret, which can be independent
  // of the text.
  EditableLayout::CaretFormat caretFormat_;

  // Mouse manipulation
  bool currentlySelecting_;
  bool currentlyPanning_;
  float previousMouseX;
  float previousMouseY;

  static constexpr auto MouseScrollFactor = 10;

  // Current view
  float scaleX_;  // horizontal scaling
  float scaleY_;  // vertical scaling
  float angle_;   // in degrees
  float originX_; // focused point in document (moves on panning and caret navigation)
  float originY_;
};
