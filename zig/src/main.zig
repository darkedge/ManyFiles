const std = @import("std");
const expect = std.testing.expect;
usingnamespace std.os.windows;
usingnamespace std.os.windows.user32;

pub extern "user32" fn DefWindowProcW(HWND, Msg: UINT, WPARAM, LPARAM) callconv(WINAPI) LRESULT;
pub extern "user32" fn CreateWindowExW(dwExStyle: DWORD, lpClassName: LPCWSTR, lpWindowName: LPCWSTR, dwStyle: DWORD, X: i32, Y: i32, nWidth: i32, nHeight: i32, hWndParent: ?HWND, hMenu: ?HMENU, hInstance: HINSTANCE, lpParam: ?LPVOID) callconv(WINAPI) ?HWND;
pub extern "user32" fn GetMessageW(lpMsg: *MSG, hWnd: ?HWND, wMsgFilterMin: UINT, wMsgFilterMax: UINT) callconv(WINAPI) BOOL;
pub extern "user32" fn DispatchMessageW(lpMsg: *const MSG) callconv(WINAPI) LRESULT;
pub extern "kernel32" fn GetLastError() callconv(WINAPI) DWORD;
pub extern "kernel32" fn RegisterClassExW(*const WNDCLASSEXW) callconv(WINAPI) ATOM;

const WS_POPUP = 0x80000000;
const WS_CHILD = 0x40000000;
const WS_MINIMIZE = 0x20000000;
const WS_VISIBLE = 0x10000000;
const WS_DISABLED = 0x08000000;
const WS_CLIPSIBLINGS = 0x04000000;
const WS_CLIPCHILDREN = 0x02000000;
const WS_MAXIMIZE = 0x01000000;
const WS_BORDER = 0x00800000;
const WS_DLGFRAME = 0x00400000;
const WS_VSCROLL = 0x00200000;
const WS_HSCROLL = 0x00100000;
const WS_GROUP = 0x00020000;
const WS_TABSTOP = 0x00010000;

const WS_TILED = WS_OVERLAPPED;
const WS_ICONIC = WS_MINIMIZE;
const WS_SIZEBOX = WS_THICKFRAME;
const WS_TILEDWINDOW = WS_OVERLAPPEDWINDOW;

const WS_OVERLAPPEDWINDOW = (WS_OVERLAPPED |
    WS_CAPTION |
    WS_SYSMENU |
    WS_THICKFRAME |
    WS_MINIMIZEBOX |
    WS_MAXIMIZEBOX);

const CW_USEDEFAULT = @truncate(i32, 0x80000000);

const ATOM = WORD;

const WNDCLASSEXW = extern struct {
    cbSize: UINT = @sizeOf(WNDCLASSEXW),
    style: UINT,
    lpfnWndProc: WNDPROC,
    cbClsExtra: i32,
    cbWndExtra: i32,
    hInstance: HINSTANCE,
    hIcon: ?HICON,
    hCursor: ?HCURSOR,
    hbrBackground: ?HBRUSH,
    lpszMenuName: ?LPCWSTR,
    lpszClassName: LPCWSTR,
    hIconSm: ?HICON,
};

fn WindowProc(hwnd: HWND, message: UINT, wParam: WPARAM, lParam: LPARAM) callconv(WINAPI) LRESULT {
    switch (message) {
        WM_CREATE => std.log.info("All your codebase are belong to us.", .{}),
        WM_DESTROY => {
            PostQuitMessage(0);
            return null;
        },
        else => {},
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

pub fn main() anyerror!void {
    const hInstance = @intToPtr(HINSTANCE, std.process.getBaseAddress());
    const className = std.unicode.utf8ToUtf16LeStringLiteral("Class Name");
    const wc = WNDCLASSEXW{
        .style = 0,
        .lpfnWndProc = WindowProc,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = hInstance,
        .hIcon = null,
        .hCursor = null,
        .hbrBackground = null,
        .lpszMenuName = null,
        .lpszClassName = className,
        .hIconSm = null,
    };
    expect(RegisterClassExW(&wc) != 0);

    const hWnd = CreateWindowExW(
        0,
        className,
        std.unicode.utf8ToUtf16LeStringLiteral("Window Title"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        null,
        null,
        hInstance,
        null, // TODO: Add application pointer
    ).?;

    _ = ShowWindow(hWnd, SW_SHOW);

    var msg: MSG = undefined;
    while (GetMessageW(&msg, null, 0, 0) != 0) {
        _ = TranslateMessage(&msg);
        _ = DispatchMessageW(&msg);
    }
}
