/*
 * Munux Kernel - Memory Management
 * 
 * Sistema completo de gerenciamento de memória incluindo:
 * - Physical Memory Manager (frames de 4KB)
 * - Virtual Memory Manager (paginação)
 * - Heap allocator (malloc/free)
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "../kernel.h"

// Tamanho de página (4KB)
#define PAGE_SIZE 4096

// Flags de página
#define PAGE_PRESENT   0x1
#define PAGE_WRITE     0x2
#define PAGE_USER      0x4
#define PAGE_ACCESSED  0x20
#define PAGE_DIRTY     0x40

// Estrutura de entrada de page table
typedef uint32_t page_t;

// Estrutura de page directory
typedef struct {
    page_t tables[1024];
} page_directory_t;

// Physical Memory Manager
void pmm_init(uint32_t mem_size);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t frame);
uint32_t pmm_get_total_memory(void);
uint32_t pmm_get_free_memory(void);

// Virtual Memory Manager
void vmm_init(void);
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
void vmm_unmap_page(uint32_t virtual_addr);
uint32_t vmm_get_physical_address(uint32_t virtual_addr);
page_directory_t* vmm_get_current_directory(void);
void vmm_switch_directory(page_directory_t* dir);

// Heap allocator
void heap_init(void);
void* kmalloc(size_t size);
void* kmalloc_aligned(size_t size);
void* kmalloc_physical(size_t size, uint32_t* phys_addr);
void kfree(void* ptr);

// Funções auxiliares
void* memset(void* dest, int val, size_t len);
void* memcpy(void* dest, const void* src, size_t len);
int memcmp(const void* s1, const void* s2, size_t n);

#endif // MEMORY_H
