const std = @import("std");
usingnamespace std.os.windows;
usingnamespace std.os.windows.user32;

const expect = std.testing.expect;

const win32 = @import("win32.zig");

fn WindowProc(hwnd: HWND, message: UINT, wParam: WPARAM, lParam: LPARAM) callconv(.Stdcall) LRESULT {
    switch (message) {
        WM_CREATE => std.log.info("All your codebase are belong to us.", .{}),
        WM_DESTROY => {
            PostQuitMessage(0);
            return null;
        },
        else => {},
    }

    return win32.DefWindowProcW(hwnd, message, wParam, lParam);
}

pub fn main() anyerror!void {
    const hInstance = @intToPtr(HINSTANCE, std.process.getBaseAddress());
    const className = std.unicode.utf8ToUtf16LeStringLiteral("Class Name");
    const wc = win32.WNDCLASSEXW{
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
    expect(win32.RegisterClassExW(&wc) != 0);

    const hWnd = win32.CreateWindowExW(
        0,
        className,
        std.unicode.utf8ToUtf16LeStringLiteral("Window Title"),
        win32.WS_OVERLAPPEDWINDOW,
        win32.CW_USEDEFAULT,
        win32.CW_USEDEFAULT,
        win32.CW_USEDEFAULT,
        win32.CW_USEDEFAULT,
        null,
        null,
        hInstance,
        null, // TODO: Add application pointer
    ).?;

    _ = win32.ShowWindow(hWnd, SW_SHOW);

    var msg: MSG = undefined;
    while (win32.GetMessageW(&msg, null, 0, 0) != 0) {
        _ = win32.TranslateMessage(&msg);
        _ = win32.DispatchMessageW(&msg);
    }
}
