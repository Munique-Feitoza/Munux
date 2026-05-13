/*
 * Munux Kernel - Physical Memory Manager
 *
 * Bitmap-based frame allocator. One bit per 4 KiB frame.
 */

#include "memory.h"

// Bitmap placement: chosen to land inside the multiboot section's
// 4 KiB page (which is otherwise pure padding after the 12-byte header
// at link time). When memory discovery moves to multiboot info in v0.4
// the bitmap will move into .bss with an appropriate size bound.
#define BITMAP_BASE_ADDR    0x100000u
#define BITS_PER_WORD       32u

// Reserved low memory (kernel image + bitmap page + early data).
#define KERNEL_RESERVED     0x200000u

#define FRAMES_NOT_FOUND    ((uint32_t)-1)

static uint32_t* const frame_bitmap = (uint32_t*)BITMAP_BASE_ADDR;
static uint32_t total_frames = 0;
static uint32_t used_frames  = 0;

static inline uint32_t addr_to_frame(uint32_t addr) {
    return addr / PAGE_SIZE;
}

static inline uint32_t bitmap_words(void) {
    return (total_frames + BITS_PER_WORD - 1) / BITS_PER_WORD;
}

static inline void mark_frame_used(uint32_t frame) {
    frame_bitmap[frame / BITS_PER_WORD] |= 1u << (frame % BITS_PER_WORD);
}

static inline void mark_frame_free(uint32_t frame) {
    frame_bitmap[frame / BITS_PER_WORD] &= ~(1u << (frame % BITS_PER_WORD));
}

// Returns FRAMES_NOT_FOUND when the bitmap is fully allocated.
static uint32_t find_free_frame(void) {
    const uint32_t words = bitmap_words();
    for (uint32_t w = 0; w < words; w++) {
        const uint32_t word = frame_bitmap[w];
        if (word == 0xFFFFFFFFu) {
            continue;
        }
        for (uint32_t bit = 0; bit < BITS_PER_WORD; bit++) {
            if (!(word & (1u << bit))) {
                return w * BITS_PER_WORD + bit;
            }
        }
    }
    return FRAMES_NOT_FOUND;
}

void pmm_init(uint32_t mem_size) {
    total_frames = mem_size / PAGE_SIZE;

    // Zero the whole bitmap in one pass.
    const uint32_t words = bitmap_words();
    for (uint32_t w = 0; w < words; w++) {
        frame_bitmap[w] = 0;
    }

    // Bulk-set the first KERNEL_RESERVED bytes worth of frames as used.
    // Setting whole words is O(reserved / PAGE_SIZE / BITS_PER_WORD)
    // instead of a per-frame bit-twiddle loop.
    const uint32_t reserved_frames = KERNEL_RESERVED / PAGE_SIZE;
    const uint32_t full_words      = reserved_frames / BITS_PER_WORD;
    const uint32_t tail_bits       = reserved_frames % BITS_PER_WORD;

    for (uint32_t w = 0; w < full_words; w++) {
        frame_bitmap[w] = 0xFFFFFFFFu;
    }
    if (tail_bits) {
        frame_bitmap[full_words] = (1u << tail_bits) - 1u;
    }
    used_frames = reserved_frames;
}

uint32_t pmm_alloc_frame(void) {
    const uint32_t frame = find_free_frame();
    if (frame == FRAMES_NOT_FOUND) {
        return 0; // 0 doubles as "out of memory" since frame 0 is reserved.
    }
    mark_frame_used(frame);
    used_frames++;
    return frame * PAGE_SIZE;
}

void pmm_free_frame(uint32_t frame_addr) {
    mark_frame_free(addr_to_frame(frame_addr));
    if (used_frames > 0) {
        used_frames--;
    }
}

uint32_t pmm_get_total_memory(void) {
    return total_frames * PAGE_SIZE;
}

uint32_t pmm_get_free_memory(void) {
    return (total_frames - used_frames) * PAGE_SIZE;
}
