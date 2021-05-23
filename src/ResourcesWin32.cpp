#include "pch.h"
// Mouse cursors, et cetera.
// Stored in static memory, loaded at run-time in the main thread.
#include "ResourcesWin32.h"
#include "ErrorExit.h"

static HCURSOR s_Cursors[res::win32::ECursor::COUNT];

void res::win32::Init()
{
  // To use one of the predefined cursors, the application must set
  // the hInstance parameter to NULL and the lpCursorName parameter
  // to one the following values (...)
  MJ_ERR_IF(s_Cursors[ECursor::Arrow] = ::LoadCursorW(nullptr, IDC_ARROW), nullptr);
  MJ_ERR_IF(s_Cursors[ECursor::Vertical] = ::LoadCursorW(nullptr, IDC_SIZENS), nullptr);
  MJ_ERR_IF(s_Cursors[ECursor::Horizontal] = ::LoadCursorW(nullptr, IDC_SIZEWE), nullptr);
}

/// <summary>
/// We allow to set the cursor directly here, as the only reason to get a
/// cursor is to immediately set it right after (for now).
/// </summary>
void res::win32::SetCursor(res::win32::ECursor::Enum cursor)
{
  if (cursor < ECursor::COUNT)
  {
    HCURSOR pCursor = s_Cursors[cursor];
    // If pCursor is NULL, the cursor is removed from the screen.
    // We have no use for this yet.
    if (pCursor)
    {
      // The return value is the handle to the previous cursor, if there was one.
      // If there was no previous cursor, the return value is NULL.
      static_cast<void>(::SetCursor(pCursor));
    }
  }
}

void res::win32::Destroy()
{
  // Note: Do NOT call DestroyCursor, as the cursors that we load are "shared",
  // which means that they must stay valid.
}
