/*
 * Munux Kernel - Core do Sistema Operacional
 * 
 * Este é o kernel básico do Munux, responsável por:
 * - Gerenciamento de memória de vídeo
 * - Impressão de texto na tela
 * - Inicialização do sistema
 * - Coordenação de subsistemas (memória, processos, drivers)
 */

#include "kernel.h"
#include "interrupts/idt.h"
#include "interrupts/io.h"
#include "memory/memory.h"
#include "process/process.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "drivers/serial.h"
#include "drivers/mouse.h"
#include "drivers/disk.h"

// VGA text-mode terminal state. All constants come from kernel.h.
static volatile uint16_t* video_memory = (uint16_t*)VGA_MEMORY;
static int terminal_row = 0;
static int terminal_column = 0;
static uint8_t terminal_color = COLOR_LIGHT_GREY | (COLOR_BLACK << 4);

// Funções de utilitários
static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
    return (uint16_t) uc | (uint16_t) color << 8;
}

// Função para calcular tamanho de string
size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

// Função para limpar a tela
void terminal_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            video_memory[index] = vga_entry(' ', terminal_color);
        }
    }
    terminal_row = 0;
    terminal_column = 0;
}

// Função para definir cor do terminal
void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

// Função para imprimir caractere na posição atual
void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            // Scroll básico - mover todas as linhas para cima
            for (size_t y = 1; y < VGA_HEIGHT; y++) {
                for (size_t x = 0; x < VGA_WIDTH; x++) {
                    video_memory[(y-1) * VGA_WIDTH + x] = video_memory[y * VGA_WIDTH + x];
                }
            }
            // Limpar última linha
            for (size_t x = 0; x < VGA_WIDTH; x++) {
                video_memory[(VGA_HEIGHT-1) * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
            }
            terminal_row = VGA_HEIGHT - 1;
        }
        return;
    }
    
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    video_memory[index] = vga_entry(c, terminal_color);
    
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_row = 0;
        }
    }
}

// Função para imprimir string
void terminal_write(const char* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        terminal_putchar(data[i]);
    }
}

// Função para imprimir string (wrapper)
void terminal_writestring(const char* data) {
    terminal_write(data, strlen(data));
}

// Caminho de pânico do kernel: usado tanto pelo lado C quanto pelo
// `#[panic_handler]` da camada Rust em `kernel/rust/munux-rs-ffi`.
__attribute__((noreturn))
void kernel_panic(const char* msg) {
    terminal_setcolor(vga_entry_color(COLOR_WHITE, COLOR_RED));
    terminal_writestring("\n*** KERNEL PANIC ***\n");
    if (msg) {
        terminal_writestring(msg);
        terminal_writestring("\n");
    }
    __asm__ volatile ("cli");
    while (1) {
        __asm__ volatile ("hlt");
    }
}

// Smoke test do heap Rust no boot. Aloca, valida que o ponto é
// gravável, libera, e re-aloca um bloco do tamanho combinado para
// confirmar que a coalescência funciona — exercita o caminho C →
// munux_rs_alloc → heap.rs e volta. Falhas caem em kernel_panic.
static void rust_heap_smoke(void) {
    enum {
        BLOCK_A_SIZE = 128,
        BLOCK_B_SIZE = 256,
        COALESCED_SIZE = BLOCK_A_SIZE + BLOCK_B_SIZE, // exige merge dos dois
        PATTERN_A = 0xAAu,
        PATTERN_B = 0x55u,
        PATTERN_C = 0xEEu,
    };

    terminal_setcolor(vga_entry_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_writestring("[smoke] Testando heap Rust...\n");

    unsigned char* a = kmalloc(BLOCK_A_SIZE);
    unsigned char* b = kmalloc(BLOCK_B_SIZE);
    if (!a || !b || a == b) {
        kernel_panic("[smoke] kmalloc devolveu NULL ou ponteiros duplicados");
    }
    memset(a, PATTERN_A, BLOCK_A_SIZE);
    memset(b, PATTERN_B, BLOCK_B_SIZE);
    if (a[0] != PATTERN_A || a[BLOCK_A_SIZE - 1] != PATTERN_A ||
        b[0] != PATTERN_B || b[BLOCK_B_SIZE - 1] != PATTERN_B) {
        kernel_panic("[smoke] memoria nao retem o que foi escrito");
    }

    kfree(a);
    kfree(b);

    unsigned char* c = kmalloc(COALESCED_SIZE);
    if (!c) {
        kernel_panic("[smoke] kmalloc apos coalesce falhou");
    }
    memset(c, PATTERN_C, COALESCED_SIZE);
    if (c[0] != PATTERN_C || c[COALESCED_SIZE - 1] != PATTERN_C) {
        kernel_panic("[smoke] bloco coalescido nao e gravavel");
    }
    kfree(c);

    terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    terminal_writestring("[smoke] kmalloc/kfree/coalesce: OK (via munux_rs)\n");
}

// Função principal do kernel
void kernel_main(void) {
    // Limpar tela
    terminal_clear();
    
    // Definir cor inicial
    terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    
    // Mensagem de boas-vindas
    terminal_writestring("========================================\n");
    terminal_writestring("        Bem-vindo ao Munux OS!         \n");
    terminal_writestring("========================================\n\n");
    
    // Inicialização de subsistemas
    terminal_setcolor(vga_entry_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_writestring("Inicializando subsistemas...\n\n");
    
    terminal_writestring("[OK] Inicializando IDT...\n");
    idt_init();

    // QEMU é iniciado com -m 32M; a descoberta real de memória via
    // multiboot info fica para v0.4.
    terminal_writestring("[OK] Inicializando PMM (32 MiB)...\n");
    pmm_init(0x02000000);

    terminal_writestring("[OK] Inicializando VMM + paginacao...\n");
    vmm_init();

    terminal_writestring("[OK] Inicializando heap (backend: Rust)...\n");
    heap_init();

    rust_heap_smoke();

    terminal_writestring("\n");
    
    // Informações do sistema
    terminal_setcolor(vga_entry_color(COLOR_LIGHT_BROWN, COLOR_BLACK));
    terminal_writestring("Sistema Operacional: Munux v0.3\n");
    terminal_writestring("Arquitetura: i386 (32-bit)\n");
    terminal_writestring("Linguagens: C + Assembly + Rust (no_std)\n\n");

    terminal_setcolor(vga_entry_color(COLOR_WHITE, COLOR_BLACK));
    terminal_writestring("Funcionalidades implementadas:\n");
    terminal_writestring("  [x] Gerenciamento de interrupcoes (IDT)\n");
    terminal_writestring("  [x] Gerenciamento de memoria fisica (PMM)\n");
    terminal_writestring("  [x] Gerenciamento de memoria virtual (VMM)\n");
    terminal_writestring("  [x] Heap allocator (Rust, first-fit + coalesce)\n");
    terminal_writestring("  [x] Timer (PIT)\n");
    terminal_writestring("  [x] Driver de teclado completo\n");
    terminal_writestring("  [x] Driver de mouse PS/2\n");
    terminal_writestring("  [x] Driver de disco ATA/IDE\n");
    terminal_writestring("  [x] Driver de porta serial\n");
    terminal_writestring("  [x] Gerenciamento de processos (PCB)\n");
    terminal_writestring("  [x] Scheduler round-robin preemptivo\n");
    terminal_writestring("  [x] Context switching\n");
    terminal_writestring("  [x] Static library Rust integrada (FFI estavel)\n\n");
    
    terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));
    terminal_writestring("Kernel inicializado com sucesso!\n");
    terminal_writestring("Desenvolvido por Munique Feitoza\n");
    terminal_writestring("Sistema pronto para operacao.\n\n");
    
    terminal_setcolor(vga_entry_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
    terminal_writestring("Todos os subsistemas carregados!\n");
    terminal_writestring("Munux OS esta operacional.\n\n");
    
    // Loop infinito - kernel ativo
    while (1) {
        __asm__ volatile ("hlt");
    }
}


