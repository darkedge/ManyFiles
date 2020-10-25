#pragma once

class TextEditor
{
public:
  enum SetSelectionMode
  {
    SetSelectionModeLeft,             // cluster left
    SetSelectionModeRight,            // cluster right
    SetSelectionModeUp,               // line up
    SetSelectionModeDown,             // line down
    SetSelectionModeLeftChar,         // single character left (backspace uses it)
    SetSelectionModeRightChar,        // single character right
    SetSelectionModeLeftWord,         // single word left
    SetSelectionModeRightWord,        // single word right
    SetSelectionModeHome,             // front of line
    SetSelectionModeEnd,              // back of line
    SetSelectionModeFirst,            // very first position
    SetSelectionModeLast,             // very last position
    SetSelectionModeAbsoluteLeading,  // explicit position (for mouse click)
    SetSelectionModeAbsoluteTrailing, // explicit position, trailing edge
    SetSelectionModeAll               // select all text
  };

public:
  HRESULT static Create(HWND parentHwnd, const wchar_t* text, IDWriteTextFormat* textFormat, IDWriteFactory* factory,
                        OUT TextEditor** textEditor);

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
  void MirrorXCoordinate(IN OUT float& x);

  // Drawing/view change
  void OnDraw();
  void DrawPage(RenderTarget& target);
  void OnSize(UINT width, UINT height);
  void OnScroll(UINT message, UINT request);
  void GetCaretRect(OUT RectF& rect);
  void UpdateSystemCaret(const RectF& rect);
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
  bool SetSelection(SetSelectionMode moveMode, UINT32 advance, bool extendSelection, bool updateCaretFormat = true);
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
  void GetScale(OUT float* scaleX, OUT float* scaleY);
  void GetViewMatrix(OUT DWRITE_MATRIX* matrix) const;
  void GetInverseViewMatrix(OUT DWRITE_MATRIX* matrix) const;
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

  void GetLineMetrics(OUT mj::ArrayList<DWRITE_LINE_METRICS>& lineMetrics);
  void GetLineFromPosition(const DWRITE_LINE_METRICS* lineMetrics, // [lineCount]
                           UINT32 lineCount, UINT32 textPosition, OUT UINT32* lineOut, OUT UINT32* linePositionOut);

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
