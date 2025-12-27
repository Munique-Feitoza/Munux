/*
 * Munux Kernel - Heap Allocator
 * 
 * Implementação de malloc/free para o kernel.
 * Usa algoritmo First-Fit com lista encadeada de blocos livres.
 */

#include "memory.h"

// Endereço inicial do heap
#define HEAP_START 0xC0000000
#define HEAP_INITIAL_SIZE 0x100000 // 1MB inicial

// Bloco de memória
typedef struct heap_block {
    size_t size;
    uint32_t is_free;
    struct heap_block* next;
} heap_block_t;

// Cabeça da lista de blocos
static heap_block_t* heap_start = 0;
static uint32_t heap_end = 0;
static uint32_t heap_max = 0;

// Alinhamento
#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

// Inicializa o heap
void heap_init(void) {
    heap_start = (heap_block_t*)HEAP_START;
    heap_end = HEAP_START + HEAP_INITIAL_SIZE;
    heap_max = HEAP_START + 0x10000000; // 256MB máximo
    
    // Cria bloco inicial
    heap_start->size = HEAP_INITIAL_SIZE - sizeof(heap_block_t);
    heap_start->is_free = 1;
    heap_start->next = 0;
    
    // Mapeia memória inicial do heap
    for (uint32_t i = HEAP_START; i < heap_end; i += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        vmm_map_page(i, frame, PAGE_PRESENT | PAGE_WRITE);
    }
}

// Expande o heap
static void expand_heap(size_t min_size) {
    size_t expand_size = ALIGN(min_size, PAGE_SIZE);
    
    if (heap_end + expand_size > heap_max) {
        return; // Heap no limite
    }
    
    // Mapeia novas páginas
    for (uint32_t i = heap_end; i < heap_end + expand_size; i += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        vmm_map_page(i, frame, PAGE_PRESENT | PAGE_WRITE);
    }
    
    heap_end += expand_size;
}

// Aloca memória (malloc)
void* kmalloc(size_t size) {
    if (size == 0) return 0;
    
    size = ALIGN(size, 4); // Alinha em 4 bytes
    
    heap_block_t* current = heap_start;
    
    // First-Fit: procura primeiro bloco livre suficiente
    while (current) {
        if (current->is_free && current->size >= size) {
            // Divide bloco se sobrar espaço
            if (current->size > size + sizeof(heap_block_t) + 16) {
                heap_block_t* new_block = (heap_block_t*)((uint32_t)current + sizeof(heap_block_t) + size);
                new_block->size = current->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = current->next;
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            return (void*)((uint32_t)current + sizeof(heap_block_t));
        }
        
        if (!current->next) {
            // Chegou ao fim sem encontrar bloco
            // Expande heap
            size_t needed = size + sizeof(heap_block_t);
            expand_heap(needed);
            
            heap_block_t* new_block = (heap_block_t*)(heap_end - needed);
            new_block->size = size;
            new_block->is_free = 0;
            new_block->next = 0;
            current->next = new_block;
            
            return (void*)((uint32_t)new_block + sizeof(heap_block_t));
        }
        
        current = current->next;
    }
    
    return 0;
}

// Aloca memória alinhada em página
void* kmalloc_aligned(size_t size) {
    void* ptr = kmalloc(size + PAGE_SIZE);
    if (!ptr) return 0;
    
    uint32_t addr = (uint32_t)ptr;
    uint32_t aligned = ALIGN(addr, PAGE_SIZE);
    
    if (aligned != addr) {
        return (void*)aligned;
    }
    
    return ptr;
}

// Aloca memória e retorna endereço físico
void* kmalloc_physical(size_t size, uint32_t* phys_addr) {
    void* virt = kmalloc(size);
    if (virt && phys_addr) {
        *phys_addr = vmm_get_physical_address((uint32_t)virt);
    }
    return virt;
}

// Libera memória (free)
void kfree(void* ptr) {
    if (!ptr) return;
    
    heap_block_t* block = (heap_block_t*)((uint32_t)ptr - sizeof(heap_block_t));
    block->is_free = 1;
    
    // Coalescência: junta blocos livres adjacentes
    heap_block_t* current = heap_start;
    while (current && current->next) {
        if (current->is_free && current->next->is_free) {
            current->size += sizeof(heap_block_t) + current->next->size;
            current->next = current->next->next;
        } else {
            current = current->next;
        }
    }
}
