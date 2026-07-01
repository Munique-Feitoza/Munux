//! Kernel heap allocator — Rust port.
//!
//! First-fit free list with adjacent-block coalescing. The on-disk
//! header layout (`#[repr(C)]`) matches the legacy `heap_block_t`
//! byte-for-byte so any debugger script that walked the C list keeps
//! working.
//!
//! Heap growth delegates to the C side via the [`munux_c_heap_grow`]
//! callback because it needs the PMM and VMM, which remain in C.

use core::ptr::{self, NonNull};

use crate::sync::IrqMutex;

#[repr(C)]
struct Block {
    size: usize,
    is_free: u32,
    next: *mut Block,
}

const HEADER_SIZE: usize = core::mem::size_of::<Block>();
const MIN_SPLIT_REMAINDER: usize = HEADER_SIZE + 16;
const DEFAULT_ALIGN: usize = 4;

extern "C" {
    /// Grow the heap by at least `min_bytes`, mapping new pages.
    /// Returns the new heap end virtual address, or 0 on failure.
    fn munux_c_heap_grow(min_bytes: usize) -> usize;
}

/// A live heap. Construction can only happen through [`init`], which
/// is why the type is unconditionally valid — there is no "uninit"
/// sentinel inside the struct itself.
struct Heap {
    head: NonNull<Block>,
    end: usize,
    max: usize,
}

// SAFETY: every access goes through HEAP.lock(); the lock serializes
// the single-owner free list.
unsafe impl Send for Heap {}

impl Heap {
    /// # Safety
    /// `start..start+initial_size` must be a contiguous, page-aligned,
    /// mapped writable region. `start+max` must be an upper bound the
    /// C side is willing to map on demand.
    unsafe fn new(start: usize, initial_size: usize, max: usize) -> Self {
        debug_assert!(initial_size > HEADER_SIZE);
        debug_assert!(max >= initial_size);

        let head = start as *mut Block;
        // SAFETY: precondition guarantees the region is writable.
        unsafe {
            (*head).size = initial_size - HEADER_SIZE;
            (*head).is_free = 1;
            (*head).next = ptr::null_mut();
        }
        // SAFETY: head is non-null because `start` is a real address.
        let head = unsafe { NonNull::new_unchecked(head) };
        Self {
            head,
            end: start + initial_size,
            max: start + max,
        }
    }

    fn allocate(&mut self, size: usize) -> *mut u8 {
        if size == 0 {
            return ptr::null_mut();
        }
        let needed = align_up(size, DEFAULT_ALIGN);
        let mut current = self.head.as_ptr();

        loop {
            // SAFETY: `current` is a valid block header by list invariant.
            let block = unsafe { &mut *current };

            if block.is_free == 1 && block.size >= needed {
                Self::split_if_worthwhile(block, needed);
                block.is_free = 0;
                return payload_of(current);
            }

            if block.next.is_null() {
                return self.grow_and_attach(block, needed);
            }
            current = block.next;
        }
    }

    /// # Safety
    /// `ptr` must have been returned by a previous [`Heap::allocate`]
    /// on this same heap, and must not have been freed since.
    #[allow(clippy::cast_ptr_alignment)]
    unsafe fn deallocate(&mut self, ptr: NonNull<u8>) {
        // SAFETY: by precondition the byte before `ptr` is a valid
        // block header produced by this allocator at Block alignment.
        let header_ptr: *mut Block = unsafe { ptr.as_ptr().sub(HEADER_SIZE).cast::<Block>() };

        // Walk once to find the predecessor while we have the lock.
        let prev = self.find_predecessor(header_ptr);

        // SAFETY: header_ptr is a valid block header.
        unsafe { (*header_ptr).is_free = 1 };

        Self::merge_with_next_if_free(header_ptr);
        if let Some(p) = prev {
            Self::merge_with_next_if_free(p.as_ptr());
        }
    }

    /// Allocate `size` bytes aligned to `align` (a power of two, rounded up to
    /// at least [`DEFAULT_ALIGN`]).
    ///
    /// Over-allocates through [`Heap::allocate`], hands back an aligned address
    /// inside that block, and stashes the base payload pointer in the word just
    /// before it so [`Heap::deallocate_aligned`] can recover the real header.
    /// Wastes at most `align + word` bytes — far less than rounding a whole page.
    fn allocate_aligned(&mut self, size: usize, align: usize) -> *mut u8 {
        if size == 0 || !align.is_power_of_two() {
            return ptr::null_mut();
        }
        let align = align.max(DEFAULT_ALIGN);
        let word = core::mem::size_of::<usize>();
        // Room for alignment slack (up to align-1) plus the back-pointer word.
        let Some(total) = size.checked_add(align).and_then(|v| v.checked_add(word)) else {
            return ptr::null_mut();
        };
        let base = self.allocate(total);
        if base.is_null() {
            return ptr::null_mut();
        }
        let base_addr = base as usize;
        // First aligned address that still leaves room for the back-pointer.
        let aligned = align_up(base_addr + word, align);
        // SAFETY: `word <= aligned - base_addr`, and `aligned + size` stays
        // inside the `total`-byte region, so both the slot and the payload are
        // within the allocation. `aligned - word` is `usize`-aligned because
        // `align >= word` and `aligned` is a multiple of `align`.
        unsafe {
            *((aligned - word) as *mut usize) = base_addr;
        }
        aligned as *mut u8
    }

    /// # Safety
    /// `ptr` must have been returned by [`Heap::allocate_aligned`] on this heap
    /// and not yet freed.
    unsafe fn deallocate_aligned(&mut self, ptr: NonNull<u8>) {
        let word = core::mem::size_of::<usize>();
        // SAFETY: `allocate_aligned` wrote the base payload address in the word
        // immediately before `ptr`.
        let base = unsafe { *((ptr.as_ptr() as usize - word) as *const usize) };
        if let Some(nn) = NonNull::new(base as *mut u8) {
            // SAFETY: `base` came from `self.allocate`; valid and not yet freed.
            unsafe { self.deallocate(nn) };
        }
    }

    /// Allocate a fresh page from the C side, write a new block header
    /// at the previous heap end, and append it after `tail`.
    ///
    /// The Rust side enforces its own `max` independently of the C
    /// grow callback as defence-in-depth.
    fn grow_and_attach(&mut self, tail: &mut Block, needed: usize) -> *mut u8 {
        let required = needed + HEADER_SIZE;
        if self.end.checked_add(required).is_none_or(|e| e > self.max) {
            return ptr::null_mut();
        }
        // SAFETY: extern call into the kernel grow callback.
        let new_end = unsafe { munux_c_heap_grow(required) };
        if new_end == 0 || new_end <= self.end || new_end > self.max {
            return ptr::null_mut();
        }

        let block_addr = self.end;
        self.end = new_end;
        let new_block = block_addr as *mut Block;

        // SAFETY: the region just returned by the grow callback is
        // mapped writable and at least `required` bytes long.
        unsafe {
            (*new_block).size = needed;
            (*new_block).is_free = 0;
            (*new_block).next = ptr::null_mut();
        }
        tail.next = new_block;
        payload_of(new_block)
    }

    #[allow(clippy::cast_ptr_alignment)]
    fn split_if_worthwhile(block: &mut Block, requested: usize) {
        if block.size <= requested + MIN_SPLIT_REMAINDER {
            return;
        }
        // SAFETY: the new header lands strictly inside the block's
        // payload region. Alignment holds because `requested` is
        // `DEFAULT_ALIGN`-aligned and the current header already sits
        // at `align_of::<Block>()`; the resulting offset is a multiple
        // of `align_of::<Block>()` too.
        let new_header: *mut Block = unsafe { payload_of(block).add(requested).cast::<Block>() };
        // SAFETY: writes are within the same owned region.
        unsafe {
            (*new_header).size = block.size - requested - HEADER_SIZE;
            (*new_header).is_free = 1;
            (*new_header).next = block.next;
        }
        block.size = requested;
        block.next = new_header;
    }

    /// Returns the block whose `next` points at `target`, or `None`
    /// if `target` is the head. The free list is sorted by address
    /// and singly-linked, so this is a single forward walk.
    fn find_predecessor(&self, target: *mut Block) -> Option<NonNull<Block>> {
        if self.head.as_ptr() == target {
            return None;
        }
        let mut current = self.head.as_ptr();
        loop {
            // SAFETY: `current` is a valid block header by invariant.
            let next = unsafe { (*current).next };
            if next == target {
                // SAFETY: current is non-null inside the walk.
                return Some(unsafe { NonNull::new_unchecked(current) });
            }
            debug_assert!(!next.is_null(), "target not in list");
            current = next;
        }
    }

    /// If both `block` and `block.next` are free, absorb the successor.
    /// Runs in O(1).
    fn merge_with_next_if_free(block: *mut Block) {
        // SAFETY: `block` is a valid header by caller contract.
        let b = unsafe { &mut *block };
        if b.is_free != 1 || b.next.is_null() {
            return;
        }
        // SAFETY: `b.next` is a valid header by list invariant.
        let n = unsafe { &mut *b.next };
        if n.is_free != 1 {
            return;
        }
        b.size += HEADER_SIZE + n.size;
        b.next = n.next;
    }
}

const fn align_up(value: usize, align: usize) -> usize {
    (value + align - 1) & !(align - 1)
}

fn payload_of<T>(header: *mut T) -> *mut u8 {
    // SAFETY: callers always pass a valid header; the payload starts
    // immediately after.
    unsafe { header.cast::<u8>().add(HEADER_SIZE) }
}

static HEAP: IrqMutex<Option<Heap>> = IrqMutex::new(None);

/// Public initialization entry point called from C after the initial
/// heap region has been mapped.
///
/// # Safety
/// See [`Heap::new`]. Must be called exactly once.
pub unsafe fn init(start: usize, initial_size: usize, max: usize) {
    // SAFETY: precondition forwarded verbatim.
    let heap = unsafe { Heap::new(start, initial_size, max) };
    *HEAP.lock() = Some(heap);
}

/// Allocate `size` bytes. Returns a null pointer on failure.
#[must_use]
pub fn alloc(size: usize) -> *mut u8 {
    match HEAP.lock().as_mut() {
        Some(heap) => heap.allocate(size),
        None => ptr::null_mut(),
    }
}

/// # Safety
/// `ptr` must have been returned by [`alloc`] and not yet freed.
pub unsafe fn dealloc(ptr: *mut u8) {
    let Some(non_null) = NonNull::new(ptr) else {
        return;
    };
    if let Some(heap) = HEAP.lock().as_mut() {
        // SAFETY: precondition forwarded; the lock serializes access.
        unsafe { heap.deallocate(non_null) };
    }
}

/// Allocate `size` bytes aligned to `align`. Returns null on failure.
#[must_use]
pub fn alloc_aligned(size: usize, align: usize) -> *mut u8 {
    match HEAP.lock().as_mut() {
        Some(heap) => heap.allocate_aligned(size, align),
        None => ptr::null_mut(),
    }
}

/// # Safety
/// `ptr` must have been returned by [`alloc_aligned`] and not yet freed.
pub unsafe fn dealloc_aligned(ptr: *mut u8) {
    let Some(non_null) = NonNull::new(ptr) else {
        return;
    };
    if let Some(heap) = HEAP.lock().as_mut() {
        // SAFETY: precondition forwarded; the lock serializes access.
        unsafe { heap.deallocate_aligned(non_null) };
    }
}
