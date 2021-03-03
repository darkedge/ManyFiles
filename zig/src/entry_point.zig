const main_window = @import("main_window.zig");

pub fn main() anyerror!void {
    const mw = main_window.MainWindow;
    mw.run();
}
