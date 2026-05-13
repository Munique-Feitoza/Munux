//! Rust panic handler routed through the existing C panic path.
//!
//! A Rust panic must surface identically to a C `kernel_panic` call.
//! The handler writes a short diagnostic banner to COM1 (so the
//! message is visible even when the VGA console is in a bad state)
//! and then hands off to the C-side panic routine which halts the CPU.

use core::panic::PanicInfo;

const COM1: u16 = 0x3F8;

extern "C" {
    fn serial_writestring(port: u16, s: *const u8);
    fn kernel_panic(msg: *const u8) -> !;
}

#[panic_handler]
fn panic(_info: &PanicInfo<'_>) -> ! {
    // SAFETY: both functions are kernel entry points whose only
    // requirement is a NUL-terminated C string. `c""` literals are
    // NUL-terminated by the compiler.
    unsafe {
        serial_writestring(
            COM1,
            c"[RUST PANIC] entering kernel_panic\n".as_ptr().cast(),
        );
        kernel_panic(c"Rust panic".as_ptr().cast())
    }
}
