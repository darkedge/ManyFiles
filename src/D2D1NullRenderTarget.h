#pragma once

// Null object implementation of ID2D1RenderTarget
// This object can be used while the D2D1 device is not yet available
struct D2D1NullRenderTarget : public ID2D1RenderTarget
{
  // IUnknown
  HRESULT QueryInterface(const IID&, void**)
  {
    return S_OK;
  }
  ULONG AddRef()
  {
    return 0;
  }
  ULONG Release()
  {
    return 0;
  }

  // ID2D1Resource
  void GetFactory(ID2D1Factory**) const
  {
  }

  // ID2D1RenderTarget
  HRESULT CreateBitmap(D2D1_SIZE_U, const void*, UINT32, const D2D1_BITMAP_PROPERTIES*, ID2D1Bitmap**)
  {
    return S_OK;
  }
  HRESULT CreateBitmapFromWicBitmap(IWICBitmapSource*, const D2D1_BITMAP_PROPERTIES*, ID2D1Bitmap**)
  {
    return S_OK;
  }
  HRESULT CreateSharedBitmap(const IID&, void*, const D2D1_BITMAP_PROPERTIES*, ID2D1Bitmap**)
  {
    return S_OK;
  }
  HRESULT CreateBitmapBrush(ID2D1Bitmap*, const D2D1_BITMAP_BRUSH_PROPERTIES*, const D2D1_BRUSH_PROPERTIES*,
                            ID2D1BitmapBrush**)
  {
    return S_OK;
  }
  HRESULT CreateSolidColorBrush(const D2D1_COLOR_F*, const D2D1_BRUSH_PROPERTIES*, ID2D1SolidColorBrush**)
  {
    return S_OK;
  }
  HRESULT CreateGradientStopCollection(const D2D1_GRADIENT_STOP*, UINT32, D2D1_GAMMA, D2D1_EXTEND_MODE,
                                       ID2D1GradientStopCollection**)
  {
    return S_OK;
  }
  HRESULT CreateLinearGradientBrush(const D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES*, const D2D1_BRUSH_PROPERTIES*,
                                    ID2D1GradientStopCollection*, ID2D1LinearGradientBrush**)
  {
    return S_OK;
  }
  HRESULT CreateRadialGradientBrush(const D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES*, const D2D1_BRUSH_PROPERTIES*,
                                    ID2D1GradientStopCollection*, ID2D1RadialGradientBrush**)
  {
    return S_OK;
  }
  HRESULT CreateCompatibleRenderTarget(const D2D1_SIZE_F*, const D2D1_SIZE_U*, const D2D1_PIXEL_FORMAT*,
                                       D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS, ID2D1BitmapRenderTarget**)
  {
    return S_OK;
  }
  HRESULT CreateLayer(const D2D1_SIZE_F*, ID2D1Layer**)
  {
    return S_OK;
  }
  HRESULT CreateMesh(ID2D1Mesh**)
  {
    return S_OK;
  }
  void DrawLine(D2D1_POINT_2F, D2D1_POINT_2F, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*)
  {
  }
  void DrawRectangle(const D2D1_RECT_F*, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*)
  {
  }
  void FillRectangle(const D2D1_RECT_F*, ID2D1Brush*)
  {
  }
  void DrawRoundedRectangle(const D2D1_ROUNDED_RECT*, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*)
  {
  }
  void FillRoundedRectangle(const D2D1_ROUNDED_RECT*, ID2D1Brush*)
  {
  }
  void DrawEllipse(const D2D1_ELLIPSE*, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*)
  {
  }
  void FillEllipse(const D2D1_ELLIPSE*, ID2D1Brush*)
  {
  }
  void DrawGeometry(ID2D1Geometry*, ID2D1Brush*, FLOAT, ID2D1StrokeStyle*)
  {
  }
  void FillGeometry(ID2D1Geometry*, ID2D1Brush*, ID2D1Brush*)
  {
  }
  void FillMesh(ID2D1Mesh*, ID2D1Brush*)
  {
  }
  void FillOpacityMask(ID2D1Bitmap*, ID2D1Brush*, D2D1_OPACITY_MASK_CONTENT, const D2D1_RECT_F*, const D2D1_RECT_F*)
  {
  }
  void DrawBitmap(ID2D1Bitmap*, const D2D1_RECT_F*, FLOAT, D2D1_BITMAP_INTERPOLATION_MODE, const D2D1_RECT_F*)
  {
  }
  void DrawTextW(const WCHAR*, UINT32, IDWriteTextFormat*, const D2D1_RECT_F*, ID2D1Brush*, D2D1_DRAW_TEXT_OPTIONS,
                 DWRITE_MEASURING_MODE)
  {
  }
  void DrawTextLayout(D2D1_POINT_2F, IDWriteTextLayout*, ID2D1Brush*, D2D1_DRAW_TEXT_OPTIONS)
  {
  }
  void DrawGlyphRun(D2D1_POINT_2F, const DWRITE_GLYPH_RUN*, ID2D1Brush*, DWRITE_MEASURING_MODE)
  {
  }
  void SetTransform(const D2D1_MATRIX_3X2_F*)
  {
  }
  void GetTransform(D2D1_MATRIX_3X2_F*) const
  {
  }
  void SetAntialiasMode(D2D1_ANTIALIAS_MODE)
  {
  }
  D2D1_ANTIALIAS_MODE GetAntialiasMode() const
  {
    return D2D1_ANTIALIAS_MODE_PER_PRIMITIVE;
  }
  void SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE)
  {
  }
  D2D1_TEXT_ANTIALIAS_MODE GetTextAntialiasMode() const
  {
    return D2D1_TEXT_ANTIALIAS_MODE_DEFAULT;
  }
  void SetTextRenderingParams(IDWriteRenderingParams*)
  {
  }
  void GetTextRenderingParams(IDWriteRenderingParams**) const
  {
  }
  void SetTags(D2D1_TAG, D2D1_TAG)
  {
  }
  void GetTags(D2D1_TAG*, D2D1_TAG*) const
  {
  }
  void PushLayer(const D2D1_LAYER_PARAMETERS*, ID2D1Layer*)
  {
  }
  void PopLayer()
  {
  }
  HRESULT Flush(D2D1_TAG*, D2D1_TAG*)
  {
    return S_OK;
  }
  void SaveDrawingState(ID2D1DrawingStateBlock*) const
  {
  }
  void RestoreDrawingState(ID2D1DrawingStateBlock*)
  {
  }
  void PushAxisAlignedClip(const D2D1_RECT_F*, D2D1_ANTIALIAS_MODE)
  {
  }
  void PopAxisAlignedClip()
  {
  }
  void Clear(const D2D1_COLOR_F*)
  {
  }
  void BeginDraw()
  {
  }
  HRESULT EndDraw(D2D1_TAG*, D2D1_TAG*)
  {
    return S_OK;
  }
  D2D1_PIXEL_FORMAT GetPixelFormat() const
  {
    return D2D1_PIXEL_FORMAT{};
  }
  void SetDpi(FLOAT, FLOAT)
  {
  }
  void GetDpi(FLOAT*, FLOAT*) const
  {
  }
  D2D1_SIZE_F GetSize() const
  {
    return D2D1_SIZE_F{};
  }
  D2D1_SIZE_U GetPixelSize() const
  {
    return D2D1_SIZE_U{};
  }
  UINT32 GetMaximumBitmapSize() const
  {
    return 0;
  }
  BOOL IsSupported(const D2D1_RENDER_TARGET_PROPERTIES*) const
  {
    return TRUE;
  }
};
