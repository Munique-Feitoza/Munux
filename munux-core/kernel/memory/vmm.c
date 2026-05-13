/*
 * Munux Kernel - Virtual Memory Manager
 * 
 * Gerencia paginação de memória virtual.
 */

#include "memory.h"

// Page directory atual
static page_directory_t* current_directory = 0;
static page_directory_t* kernel_directory = 0;

// Endereço onde page tables temporárias são mapeadas
#define TEMP_PAGE_ADDR 0xFFC00000

// Habilita paginação
static void enable_paging(page_directory_t* dir) {
    __asm__ volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "or $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n"
        : : "r"(dir)
        : "eax"
    );
}

// Inicializa VMM.
//
// Bootstrap order: PMM must be live when this runs. The page directory
// comes directly from a physical frame (4 KiB, naturally page-aligned)
// because the kernel heap is not initialized yet — it depends on the
// VMM. The pre-v0.3 code allocated the PD via `kmalloc_aligned`, which
// silently created a chicken-and-egg between VMM and the heap; this
// was harmless only because `kernel_main` never actually called these
// init functions. Fixed as part of v0.3 wrap-up.
void vmm_init(void) {
    uint32_t pd_phys = pmm_alloc_frame();
    kernel_directory = (page_directory_t*)pd_phys;
    memset(kernel_directory, 0, sizeof(page_directory_t));

    // current_directory must point at the new PD before the first
    // map_page call, so on-demand page-table allocation works.
    current_directory = kernel_directory;
    for (uint32_t i = 0; i < 0x800000; i += PAGE_SIZE) {
        vmm_map_page(i, i, PAGE_PRESENT | PAGE_WRITE);
    }

    // VGA framebuffer
    vmm_map_page(0xB8000, 0xB8000, PAGE_PRESENT | PAGE_WRITE);

    enable_paging(kernel_directory);
}

// Mapeia página virtual para física
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Aloca page table se não existir
    if (!(current_directory->tables[pd_index] & PAGE_PRESENT)) {
        uint32_t pt_phys = pmm_alloc_frame();
        current_directory->tables[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITE | flags;
        
        // Zera a page table
        page_t* pt = (page_t*)(pt_phys);
        for (int i = 0; i < 1024; i++) {
            pt[i] = 0;
        }
    }
    
    // Mapeia a página
    page_t* pt = (page_t*)(current_directory->tables[pd_index] & 0xFFFFF000);
    pt[pt_index] = (physical_addr & 0xFFFFF000) | flags;
}

// Desmapeia página virtual
void vmm_unmap_page(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    if (current_directory->tables[pd_index] & PAGE_PRESENT) {
        page_t* pt = (page_t*)(current_directory->tables[pd_index] & 0xFFFFF000);
        pt[pt_index] = 0;
        
        // Invalida TLB
        __asm__ volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
    }
}

// Obtém endereço físico de virtual
uint32_t vmm_get_physical_address(uint32_t virtual_addr) {
    uint32_t pd_index = virtual_addr >> 22;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;
    
    if (!(current_directory->tables[pd_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    page_t* pt = (page_t*)(current_directory->tables[pd_index] & 0xFFFFF000);
    if (!(pt[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    return (pt[pt_index] & 0xFFFFF000) + offset;
}

// Retorna page directory atual
page_directory_t* vmm_get_current_directory(void) {
    return current_directory;
}

// Muda page directory
void vmm_switch_directory(page_directory_t* dir) {
    current_directory = dir;
    __asm__ volatile("mov %0, %%cr3" : : "r"(dir));
}
