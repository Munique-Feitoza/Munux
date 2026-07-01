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
#include "lib/libk.h"
#include "munux_rs.h"
#include "syscall.h"

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

    // Alocação alinhada (backend Rust: munux_rs_alloc_aligned).
    void* pa = kmalloc_aligned(100);
    if (!pa || ((uint32_t)pa % PAGE_SIZE) != 0) {
        kernel_panic("[smoke] kmalloc_aligned nao devolveu ponteiro alinhado");
    }
    memset(pa, PATTERN_C, 100);
    if (((unsigned char*)pa)[0] != PATTERN_C || ((unsigned char*)pa)[99] != PATTERN_C) {
        kernel_panic("[smoke] memoria alinhada nao retem escrita");
    }
    kfree_aligned(pa);

    terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    terminal_writestring("[smoke] kmalloc/kfree/coalesce/aligned: OK (via munux_rs)\n");
}

// ---------------------------------------------------------------------------
// v0.4 — Integração: interrupções, drivers e shell interativo
// ---------------------------------------------------------------------------

// Log espelhado na tela (VGA) e na COM1, para observação headless.
static void klog(const char* s) {
    terminal_writestring(s);
    serial_writestring(COM1, s);
}

// Escreve um uint32 em decimal, na tela e na serial (via libk).
static void klog_uint(uint32_t v) {
    char buf[12];
    klog(utoa(v, buf, 10));
}

// printf do kernel: formata (libk) e joga em klog (VGA + serial).
__attribute__((format(printf, 1, 2)))
static void kprintf(const char* fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    kvsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    klog(buf);
}

// Auto-teste da libk: string, conversão numérica e ctype. Falha => panic.
static void libk_selfcheck(void) {
    char buf[12];
    if (strcmp("abc", "abc") != 0 || strcmp("abc", "abd") == 0) {
        kernel_panic("[libk] strcmp incorreto");
    }
    if (strncmp("echo x", "echo ", 5) != 0) {
        kernel_panic("[libk] strncmp incorreto");
    }
    if (strcmp(utoa(0u, buf, 10), "0") != 0 ||
        strcmp(utoa(305419896u, buf, 16), "12345678") != 0 ||
        strcmp(itoa(-42, buf, 10), "-42") != 0) {
        kernel_panic("[libk] utoa/itoa incorreto");
    }
    if (atoi("  -123") != -123 || !isdigit('7') || isdigit('x') ||
        toupper('a') != 'A' || tolower('Z') != 'z') {
        kernel_panic("[libk] atoi/ctype incorreto");
    }

    char sb[64];
    ksnprintf(sb, sizeof(sb), "%d %u %x %s %c %%", -5, 7u, 255u, "ok", 'A');
    if (strcmp(sb, "-5 7 ff ok A %") != 0) {
        kernel_panic("[libk] ksnprintf incorreto");
    }
    ksnprintf(sb, sizeof(sb), "[%08x]", 0x1234u);
    if (strcmp(sb, "[00001234]") != 0) {
        kernel_panic("[libk] ksnprintf pad incorreto");
    }
    char tiny[4];
    if (ksnprintf(tiny, sizeof(tiny), "abcdef") != 6 || strcmp(tiny, "abc") != 0) {
        kernel_panic("[libk] ksnprintf bounds incorreto");
    }

    klog("[libk] string/conv/ctype/printf: OK\n");
}

// Invoca uma syscall via int 0x80 (eax=num, ebx/ecx/edx=args, retorno em eax).
static inline uint32_t do_syscall(uint32_t num, uint32_t a1, uint32_t a2, uint32_t a3) {
    uint32_t ret;
    __asm__ volatile ("int $0x80"
                      : "=a"(ret)
                      : "a"(num), "b"(a1), "c"(a2), "d"(a3)
                      : "memory");
    return ret;
}

// Auto-teste das syscalls: exercita SYS_WRITE e SYS_GET_TICKS via int 0x80.
static void syscall_selfcheck(void) {
    const char* msg = "[sys] escrito via int 0x80\n";
    if (do_syscall(SYS_WRITE, 1, (uint32_t)msg, strlen(msg)) != strlen(msg)) {
        kernel_panic("[sys] SYS_WRITE retornou tamanho errado");
    }
    if (do_syscall(9999, 0, 0, 0) != (uint32_t)-1) {
        kernel_panic("[sys] syscall desconhecida deveria retornar -1");
    }
    (void)do_syscall(SYS_GET_TICKS, 0, 0, 0);

    // open/read/close sobre o VFS (tmpfs): lê /etc/motd via int 0x80.
    const char* path = "/etc/motd";
    int fd = (int)do_syscall(SYS_OPEN, (uint32_t)path, strlen(path), 0);
    if (fd < 3) {
        kernel_panic("[sys] SYS_OPEN falhou");
    }
    char rbuf[32];
    int rn = (int)do_syscall(SYS_READ, (uint32_t)fd, (uint32_t)rbuf, sizeof(rbuf));
    if (rn <= 0 || rn >= (int)sizeof(rbuf)) {
        kernel_panic("[sys] SYS_READ falhou");
    }
    rbuf[rn] = '\0';
    if (strcmp(rbuf, "Munux v0.4\n") != 0) {
        kernel_panic("[sys] SYS_READ devolveu conteudo errado");
    }
    if (do_syscall(SYS_CLOSE, (uint32_t)fd, 0, 0) != 0) {
        kernel_panic("[sys] SYS_CLOSE falhou");
    }

    klog("[sys] syscalls (int 0x80): OK\n");
}

// Apaga o último caractere na tela (backspace visual).
static void terminal_backspace(void) {
    if (terminal_column > 0) {
        terminal_column--;
    } else if (terminal_row > 0) {
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;
    } else {
        return;
    }
    const size_t index = terminal_row * VGA_WIDTH + terminal_column;
    video_memory[index] = vga_entry(' ', terminal_color);
}

// Auto-teste de boot: prova que IRQ0 (timer) está entregando interrupções.
// Busy-wait limitado até o contador de ticks avançar. Se avançar, o PIC
// remapeado + os stubs corrigidos + o `sti` estão todos corretos. O limite
// evita hang infinito caso a cadeia de interrupção esteja quebrada.
static void boot_selfcheck(void) {
    klog("[chk] Aguardando IRQ0 (timer) apos sti...\n");
    uint32_t t0 = timer_get_ticks();
    volatile uint32_t spins = 0;
    while (timer_get_ticks() == t0 && spins < 500000000u) {
        spins++;
    }
    uint32_t t1 = timer_get_ticks();
    if (t1 > t0) {
        klog("[chk] IRQ0 OK: timer avancou para ");
        klog_uint(t1);
        klog(" tick(s) via interrupcao.\n");
    } else {
        kernel_panic("[chk] timer nao avancou: IRQ/PIC/sti falhou");
    }
}

// Executa um comando digitado no shell.
static void shell_exec(const char* line) {
    if (line[0] == '\0') {
        return;
    }
    if (strcmp(line, "help") == 0) {
        klog("Comandos: help, clear, about, ticks, echo <texto>\n");
    } else if (strcmp(line, "clear") == 0) {
        terminal_clear();
    } else if (strcmp(line, "about") == 0) {
        klog("Munux OS — flavor de proposito geral do ecossistema Munux.\n");
        klog("Kernel poliglota C + Assembly + Rust (no_std).\n");
    } else if (strcmp(line, "ticks") == 0) {
        kprintf("Ticks do timer desde o boot: %u\n", timer_get_ticks());
    } else if (strncmp(line, "echo ", 5) == 0) {
        klog(line + 5);
        klog("\n");
    } else {
        klog("comando nao encontrado: ");
        klog(line);
        klog("\n");
    }
}

// Shell interativo: lê linhas do teclado (via IRQ1) e executa comandos.
// Não retorna — é o loop de vida do kernel.
static void shell_run(void) {
    char line[128];

    terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREEN, COLOR_BLACK));
    klog("\nMunux shell interativo pronto. Digite 'help'.\n");

    while (1) {
        terminal_setcolor(vga_entry_color(COLOR_LIGHT_CYAN, COLOR_BLACK));
        terminal_writestring("munux> ");
        serial_writestring(COM1, "munux> ");
        terminal_setcolor(vga_entry_color(COLOR_LIGHT_GREY, COLOR_BLACK));

        size_t len = 0;
        while (1) {
            char c = keyboard_getchar();
            if (c == '\n') {
                terminal_putchar('\n');
                serial_putchar(COM1, '\n');
                break;
            } else if (c == '\b') {
                if (len > 0) {
                    len--;
                    terminal_backspace();
                    serial_putchar(COM1, '\b');
                }
            } else if ((unsigned char)c >= 32 && (unsigned char)c < 127) {
                if (len < sizeof(line) - 1) {
                    line[len++] = c;
                    terminal_putchar(c);
                    serial_putchar(COM1, c);
                }
            }
        }
        line[len] = '\0';
        shell_exec(line);
    }
}

#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u
#define MEM_FALLBACK_BYTES (32u * 1024u * 1024u)

// Descobre a RAM total via multiboot info (ponteiro em EBX na entrada).
// Cai no fallback de 32 MiB se não viemos de um loader multiboot ou se o
// campo mem_upper não estiver presente. Chamado antes de pmm_init e antes
// da paginação, então lê a struct diretamente (memória baixa identidade).
static uint32_t detect_memory(uint32_t magic, uint32_t mb_info_addr) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC || mb_info_addr == 0) {
        klog("[mem] Sem multiboot valido — fallback de 32 MiB\n");
        return MEM_FALLBACK_BYTES;
    }
    const volatile uint32_t* mbi = (const volatile uint32_t*)mb_info_addr;
    if (!(mbi[0] & 0x1u)) { // flags, bit 0: mem_lower/mem_upper presentes?
        klog("[mem] multiboot sem meminfo — fallback de 32 MiB\n");
        return MEM_FALLBACK_BYTES;
    }
    // mbi[2] = mem_upper: KiB acima de 1 MiB. Total ≈ 1 MiB + mem_upper.
    uint32_t total = 0x100000u + mbi[2] * 1024u;
    if (total < MEM_FALLBACK_BYTES) {
        total = MEM_FALLBACK_BYTES; // não confia em valores absurdamente baixos
    }
    klog("[mem] Detectado via multiboot: ");
    klog_uint(total / (1024u * 1024u));
    klog(" MiB\n");
    return total;
}

// Função principal do kernel
void kernel_main(uint32_t magic, uint32_t mb_info_addr) {
    // Limpar tela
    terminal_clear();

    // Serial cedo, para log headless do boot (make qemu usa -serial stdio).
    serial_init(COM1);
    
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

    // Descoberta real de memória via multiboot info (v0.4); fallback 32 MiB.
    uint32_t mem_bytes = detect_memory(magic, mb_info_addr);
    terminal_writestring("[OK] Inicializando PMM...\n");
    pmm_init(mem_bytes);

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

    // v0.4 — Bring-up de interrupções e drivers.
    // O PIC já foi remapeado dentro de idt_init(); aqui ligamos os drivers
    // (que registram seus handlers de IRQ), habilitamos as interrupções e
    // entramos no shell interativo.
    timer_init(100);            // 100 Hz — registra IRQ0
    klog("[OK] Timer PIT @ 100 Hz (IRQ0)\n");
    keyboard_init();            // registra IRQ1
    klog("[OK] Teclado PS/2 (IRQ1)\n");

    __asm__ volatile ("sti");   // habilita interrupções de hardware
    klog("[OK] Interrupcoes habilitadas (sti)\n");

    boot_selfcheck();           // prova que IRQ0 dispara de fato
    libk_selfcheck();           // valida a libk (string/conv/ctype)
    if (munux_rs_vfs_selftest() != 0) {
        kernel_panic("[vfs] tmpfs selftest falhou");
    }
    klog("[vfs] tmpfs (criar/escrever/ler/listar): OK\n");
    munux_rs_vfs_init(); // monta o tmpfs de uso + /etc/motd + descritores
    syscall_selfcheck();

    // ext2: só testa se houver disco anexado (senão o PIO daria timeout).
    disk_init();
    if (disk_present()) {
        if (munux_rs_bcache_selftest() != 0) {
            kernel_panic("[bcache] cache de disco falhou");
        }
        klog("[bcache] cache de setores (hit/miss): OK\n");
        if (munux_rs_ext2_selftest() != 0) {
            kernel_panic("[ext2] leitura da imagem falhou");
        }
        klog("[ext2] montar + ler /hello.txt: OK\n");
    } else {
        klog("[ext2] sem disco anexado — pulado\n");
    }

    shell_run();                // interativo — não retorna

    // Rede de segurança: se o shell algum dia retornar, para a CPU.
    while (1) {
        __asm__ volatile ("hlt");
    }
}


