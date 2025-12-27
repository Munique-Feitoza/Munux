/*
 * Munux Kernel - Physical Memory Manager
 * 
 * Gerencia frames de memória física usando bitmap.
 */

#include "memory.h"

// Bitmap para frames de memória (1 bit = 1 frame de 4KB)
static uint32_t* frame_bitmap = (uint32_t*)0x100000; // 1MB
static uint32_t total_frames = 0;
static uint32_t used_frames = 0;

// Macros para manipular bitmap
#define FRAME_INDEX(addr) ((addr) / PAGE_SIZE)
#define FRAME_OFFSET(addr) ((addr) % PAGE_SIZE)
#define BITMAP_INDEX(frame) ((frame) / 32)
#define BITMAP_OFFSET(frame) ((frame) % 32)

// Marca um frame como usado
static void set_frame(uint32_t frame_addr) {
    uint32_t frame = FRAME_INDEX(frame_addr);
    uint32_t idx = BITMAP_INDEX(frame);
    uint32_t off = BITMAP_OFFSET(frame);
    frame_bitmap[idx] |= (1 << off);
}

// Marca um frame como livre
static void clear_frame(uint32_t frame_addr) {
    uint32_t frame = FRAME_INDEX(frame_addr);
    uint32_t idx = BITMAP_INDEX(frame);
    uint32_t off = BITMAP_OFFSET(frame);
    frame_bitmap[idx] &= ~(1 << off);
}

// Verifica se frame está sendo usado
static uint32_t test_frame(uint32_t frame_addr) {
    uint32_t frame = FRAME_INDEX(frame_addr);
    uint32_t idx = BITMAP_INDEX(frame);
    uint32_t off = BITMAP_OFFSET(frame);
    return frame_bitmap[idx] & (1 << off);
}

// Encontra primeiro frame livre
static uint32_t first_free_frame(void) {
    for (uint32_t i = 0; i < total_frames / 32; i++) {
        if (frame_bitmap[i] != 0xFFFFFFFF) {
            for (uint32_t j = 0; j < 32; j++) {
                if (!(frame_bitmap[i] & (1 << j))) {
                    return i * 32 + j;
                }
            }
        }
    }
    return (uint32_t)-1; // Sem frames livres
}

// Inicializa o gerenciador de memória física
void pmm_init(uint32_t mem_size) {
    total_frames = mem_size / PAGE_SIZE;
    
    // Zera bitmap
    for (uint32_t i = 0; i < total_frames / 32; i++) {
        frame_bitmap[i] = 0;
    }
    
    // Marca primeiros 2MB como usados (kernel + bitmap)
    for (uint32_t i = 0; i < 0x200000; i += PAGE_SIZE) {
        set_frame(i);
        used_frames++;
    }
}

// Aloca um frame físico
uint32_t pmm_alloc_frame(void) {
    uint32_t frame = first_free_frame();
    if (frame == (uint32_t)-1) {
        return 0; // Sem memória
    }
    
    set_frame(frame * PAGE_SIZE);
    used_frames++;
    return frame * PAGE_SIZE;
}

// Libera um frame físico
void pmm_free_frame(uint32_t frame_addr) {
    clear_frame(frame_addr);
    if (used_frames > 0) {
        used_frames--;
    }
}

// Retorna memória total
uint32_t pmm_get_total_memory(void) {
    return total_frames * PAGE_SIZE;
}

// Retorna memória livre
uint32_t pmm_get_free_memory(void) {
    return (total_frames - used_frames) * PAGE_SIZE;
}
