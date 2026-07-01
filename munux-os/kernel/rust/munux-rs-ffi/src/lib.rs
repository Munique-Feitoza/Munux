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

extern crate alloc;

use core::alloc::{GlobalAlloc, Layout};

use munux_rs::heap;

/// Global allocator que liga as coleções do `alloc` do Rust ao heap do kernel.
struct KernelAllocator;

// SAFETY: encaminha cada requisição para o heap do kernel, um alocador válido.
// O `alloc` nunca chama com layout de tamanho zero (coleções usam ponteiros
// dangling para ZSTs), e `Layout::align` é sempre potência de dois, que é o
// que `alloc_aligned` exige.
unsafe impl GlobalAlloc for KernelAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        heap::alloc_aligned(layout.size(), layout.align())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        // SAFETY: `ptr` foi devolvido por `alloc_aligned` no `alloc` acima.
        unsafe { heap::dealloc_aligned(ptr) };
    }
}

#[global_allocator]
static ALLOCATOR: KernelAllocator = KernelAllocator;

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

/// Allocate `size` bytes from the kernel heap, aligned to `align`
/// (a power of two; rounded up to at least the default alignment).
///
/// Returns a null pointer on failure. Must be freed with
/// [`munux_rs_free_aligned`], never with [`munux_rs_free`].
#[no_mangle]
pub extern "C" fn munux_rs_alloc_aligned(size: usize, align: usize) -> *mut u8 {
    heap::alloc_aligned(size, align)
}

/// Free a pointer previously returned by [`munux_rs_alloc_aligned`].
///
/// # Safety
/// `ptr` must either be null or have been returned by
/// [`munux_rs_alloc_aligned`] and not yet freed.
#[no_mangle]
pub unsafe extern "C" fn munux_rs_free_aligned(ptr: *mut u8) {
    // SAFETY: precondition forwarded to the safe wrapper.
    unsafe { heap::dealloc_aligned(ptr) };
}

/// Roda o self-test interno do VFS/tmpfs.
///
/// Retorna 0 em sucesso e diferente de zero em falha. Requer o heap já
/// inicializado.
#[no_mangle]
pub extern "C" fn munux_rs_vfs_selftest() -> i32 {
    i32::from(!munux_rs::vfs::selftest())
}

/// Roda o self-test do leitor ext2 contra o disco Primary Master.
///
/// Retorna 0 em sucesso e diferente de zero em falha. Requer uma imagem ext2
/// válida no disco.
#[no_mangle]
pub extern "C" fn munux_rs_ext2_selftest() -> i32 {
    i32::from(!munux_rs::ext2::selftest())
}

/// Roda o self-test do buffer cache (hit/miss). Requer disco anexado.
///
/// Retorna 0 em sucesso e diferente de zero em falha.
#[no_mangle]
pub extern "C" fn munux_rs_bcache_selftest() -> i32 {
    i32::from(!munux_rs::bcache::selftest())
}

/// Inicializa o VFS (monta tmpfs de uso + arquivos padrão + tabela de fds).
#[no_mangle]
pub extern "C" fn munux_rs_vfs_init() {
    munux_rs::vfs::init_default();
}

/// Abre `path` (ponteiro C + tamanho em bytes). Retorna fd (>= 3) ou -1.
///
/// # Safety
/// `path` deve apontar para `len` bytes legíveis.
#[no_mangle]
pub unsafe extern "C" fn munux_rs_vfs_open(path: *const u8, len: usize) -> i32 {
    // SAFETY: precondição do chamador (ponteiro + tamanho válidos).
    let bytes = unsafe { core::slice::from_raw_parts(path, len) };
    match core::str::from_utf8(bytes) {
        Ok(s) => munux_rs::vfs::fd_open(s),
        Err(_) => -1,
    }
}

/// Lê até `len` bytes do `fd` para `buf`. Retorna bytes lidos, 0 no EOF, -1 erro.
///
/// # Safety
/// `buf` deve apontar para `len` bytes graváveis.
#[no_mangle]
pub unsafe extern "C" fn munux_rs_vfs_read(fd: i32, buf: *mut u8, len: usize) -> i32 {
    // SAFETY: precondição do chamador (ponteiro + tamanho válidos).
    let out = unsafe { core::slice::from_raw_parts_mut(buf, len) };
    munux_rs::vfs::fd_read(fd, out)
}

/// Fecha o descritor `fd`. Retorna 0 ou -1.
#[no_mangle]
pub extern "C" fn munux_rs_vfs_close(fd: i32) -> i32 {
    munux_rs::vfs::fd_close(fd)
}
