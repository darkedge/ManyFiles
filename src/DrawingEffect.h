#pragma once

class DrawingEffect : public IUnknown
{
public:
  DrawingEffect(UINT32 color) : color_(color)
  {
  }

  inline UINT32 GetColor() const
  {
    // Returns the BGRA value for D2D.
    return color_;
  }

  inline COLORREF GetColorRef() const
  {
    // Returns color as COLORREF.
    return GetColorRef(color_);
  }

  static inline COLORREF GetColorRef(UINT32 bgra)
  {
    // Swaps color order (bgra <-> rgba) from D2D/GDI+'s to a COLORREF.
    // This also leaves the top byte 0, since alpha is ignored anyway.
    return RGB(GetBValue(bgra), GetGValue(bgra), GetRValue(bgra));
  }

  static inline COLORREF GetBgra(COLORREF rgb)
  {
    // Swaps color order (bgra <-> rgba) from COLORREF to D2D/GDI+'s.
    // Sets alpha to full opacity.
    return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb)) | 0xFF000000;
  }

  // IUnknown interface implementation
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(
      /* [in] */ REFIID riid,
      /* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
  {
    return E_NOTIMPL;
  }

  virtual ULONG STDMETHODCALLTYPE AddRef() override
  {
    return 0;
  }

  virtual ULONG STDMETHODCALLTYPE Release() override
  {
    return 0;
  }

private:
  // The color is stored as BGRA, with blue in the lowest byte,
  // then green, red, alpha; which is what D2D, GDI+, and GDI DIBs use.
  // GDI's COLORREF stores red as the lowest byte.
  UINT32 color_;
};
