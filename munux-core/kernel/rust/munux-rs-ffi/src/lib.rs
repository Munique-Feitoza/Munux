//! FFI surface of the Munux Rust subsystem.
//!
//! Every symbol exported here is part of the public C ABI consumed by
//! the C kernel. Signatures are mirrored in
//! `kernel/rust/include/munux_rs.h`, which is committed to the
//! repository and regenerated with `make rust-headers`.
//!
//! This is the only crate allowed to contain `unsafe` blocks at the
//! public API boundary. The `munux-rs` crate enforces
//! `#![forbid(unsafe_op_in_unsafe_fn)]` and keeps `unsafe` confined to
//! a small set of audited internal helpers.

#![no_std]
#![forbid(unsafe_op_in_unsafe_fn)]
#![deny(clippy::missing_safety_doc)]
#![deny(clippy::undocumented_unsafe_blocks)]

mod panic;

use munux_rs::heap;

/// Initialize the Rust-backed kernel heap.
///
/// Must be called exactly once, after the initial heap region has been
/// mapped writable by the C-side `heap_init`.
///
/// # Safety
/// `start` must point to a contiguous, page-aligned, mapped writable
/// region of length at least `initial_size`. `max` is the upper bound
/// the C side is willing to grow the heap to via `munux_c_heap_grow`.
#[no_mangle]
pub unsafe extern "C" fn munux_rs_heap_init(start: usize, initial_size: usize, max: usize) {
    // SAFETY: preconditions forwarded verbatim to the safe wrapper.
    unsafe { heap::init(start, initial_size, max) };
}

/// Allocate `size` bytes from the kernel heap.
///
/// Returns a null pointer on failure (out of memory, zero size,
/// or heap not initialized).
#[no_mangle]
pub extern "C" fn munux_rs_alloc(size: usize) -> *mut u8 {
    heap::alloc(size)
}

/// Free a pointer previously returned by [`munux_rs_alloc`].
///
/// # Safety
/// `ptr` must either be null or have been returned by
/// [`munux_rs_alloc`] and not yet freed.
#[no_mangle]
pub unsafe extern "C" fn munux_rs_free(ptr: *mut u8) {
    // SAFETY: precondition forwarded to the safe wrapper.
    unsafe { heap::dealloc(ptr) };
}
