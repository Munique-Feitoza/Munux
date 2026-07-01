/*
 * Munux Kernel - Heap Allocator (C bootstrap + Rust backend)
 *
 * Since v0.3 the heap free-list and allocation logic live in Rust
 * (`kernel/rust/munux-rs/src/heap.rs`). This file keeps the parts that
 * unavoidably touch C-side machinery — the initial page mapping via
 * PMM/VMM and the grow callback — and forwards `kmalloc` / `kfree` to
 * the Rust static library.
 *
 * The public API (kmalloc/kmalloc_aligned/kmalloc_physical/kfree)
 * is preserved byte-for-byte so existing call sites are unaffected.
 */

#include "memory.h"
#include "munux_rs.h"

// Layout: identical to the pre-v0.3 C heap so any external tooling
// (debugger scripts, doc references) keeps working unchanged.
#define HEAP_START         0xC0000000
#define HEAP_INITIAL_SIZE  0x00100000  // 1 MiB
#define HEAP_MAX_SIZE      0x10000000  // 256 MiB

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

static uint32_t heap_end = 0;
static uint32_t heap_max_addr = 0;

// Maps `[from, to)` writable into the kernel address space.
static void map_heap_range(uint32_t from, uint32_t to) {
    for (uint32_t va = from; va < to; va += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        vmm_map_page(va, frame, PAGE_PRESENT | PAGE_WRITE);
    }
}

// IRQ save/restore callbacks consumed by the Rust IrqMutex.
// These are deliberately small and self-contained so the only
// FFI dependency of `munux-rs::sync` is a pair of trivial functions.
uint32_t munux_c_irq_save(void) {
    uint32_t flags;
    __asm__ volatile (
        "pushfl\n\t"
        "popl %0\n\t"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );
    return flags;
}

void munux_c_irq_restore(uint32_t flags) {
    __asm__ volatile (
        "pushl %0\n\t"
        "popfl"
        :
        : "r"(flags)
        : "memory", "cc"
    );
}

void heap_init(void) {
    heap_end = HEAP_START + HEAP_INITIAL_SIZE;
    heap_max_addr = HEAP_START + HEAP_MAX_SIZE;

    map_heap_range(HEAP_START, heap_end);

    // Hand the region to the Rust allocator. From this point on,
    // all allocation and free decisions are made on the Rust side.
    munux_rs_heap_init(HEAP_START, HEAP_INITIAL_SIZE, HEAP_MAX_SIZE);
}

// Called from Rust when the free list runs out of space.
// Maps additional pages and returns the new heap end (0 on failure).
size_t munux_c_heap_grow(size_t min_bytes) {
    size_t aligned = ALIGN(min_bytes, PAGE_SIZE);
    uint32_t new_end = heap_end + (uint32_t)aligned;

    if (new_end > heap_max_addr) {
        return 0;
    }

    map_heap_range(heap_end, new_end);
    heap_end = new_end;
    return new_end;
}

void* kmalloc(size_t size) {
    return (void*)munux_rs_alloc(size);
}

void kfree(void* ptr) {
    munux_rs_free((uint8_t*)ptr);
}

// Alocação alinhada à página, servida pelo alocador alinhado do Rust.
// Deve ser liberada com kfree_aligned (NÃO com kfree): o ponteiro alinhado
// guarda um back-pointer para o bloco real logo antes dele.
void* kmalloc_aligned(size_t size) {
    return (void*)munux_rs_alloc_aligned(size, PAGE_SIZE);
}

void kfree_aligned(void* ptr) {
    munux_rs_free_aligned((uint8_t*)ptr);
}

void* kmalloc_physical(size_t size, uint32_t* phys_addr) {
    void* virt = kmalloc(size);
    if (virt && phys_addr) {
        *phys_addr = vmm_get_physical_address((uint32_t)virt);
    }
    return virt;
}
