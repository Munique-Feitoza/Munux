# Referência da API do Kernel Munux

## Gerenciamento de Memória

### Gerenciador de Memória Física

```c
void pmm_init(uint32_t mem_size);
```
Inicializa o gerenciador de memória física com o total de RAM do sistema.

```c
uint32_t pmm_alloc_frame(void);
```
Aloca um frame físico de 4KB. Retorna o endereço físico ou 0 se não houver memória disponível.

```c
void pmm_free_frame(uint32_t frame_addr);
```
Libera um frame previamente alocado.

```c
uint32_t pmm_get_total_memory(void);
```
Retorna a memória total do sistema em bytes.

```c
uint32_t pmm_get_free_memory(void);
```
Retorna a memória disponível em bytes.

### Gerenciador de Memória Virtual

```c
void vmm_init(void);
```
Inicializa a memória virtual com paginação habilitada.

```c
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
```
Mapeia uma página virtual para um frame físico com as flags especificadas:
- PAGE_PRESENT: Página presente na memória
- PAGE_WRITE: Página gravável
- PAGE_USER: Página acessível a partir do modo usuário

```c
void vmm_unmap_page(uint32_t virtual_addr);
```
Remove o mapeamento de uma página virtual.

```c
uint32_t vmm_get_physical_address(uint32_t virtual_addr);
```
Traduz um endereço virtual para endereço físico. Retorna 0 se não estiver mapeado.

```c
page_directory_t* vmm_get_current_directory(void);
```
Obtém o page directory atual.

```c
void vmm_switch_directory(page_directory_t* dir);
```
Troca para um page directory diferente.

### Alocador de Heap

```c
void heap_init(void);
```
Inicializa o heap do kernel.

```c
void* kmalloc(size_t size);
```
Aloca memória do heap do kernel. Retorna um ponteiro ou NULL se a alocação falhar.

```c
void* kmalloc_aligned(size_t size);
```
Aloca memória alinhada em página.

```c
void* kmalloc_physical(size_t size, uint32_t* phys_addr);
```
Aloca memória e retorna tanto o endereço virtual quanto o físico.

```c
void kfree(void* ptr);
```
Libera memória previamente alocada.

### Utilitários de Memória

```c
void* memset(void* dest, int val, size_t len);
```
Preenche uma região de memória com um valor de byte.

```c
void* memcpy(void* dest, const void* src, size_t len);
```
Copia memória da origem para o destino.

```c
int memcmp(const void* s1, const void* s2, size_t n);
```
Compara duas regiões de memória. Retorna 0 se iguais, <0 ou >0 caso contrário.

## Tratamento de Interrupções

### Gerenciamento da IDT

```c
void idt_init(void);
```
Inicializa a Interrupt Descriptor Table e reprograma o PIC.

```c
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
```
Configura uma entrada específica da IDT.

```c
void register_interrupt_handler(uint8_t n, isr_t handler);
```
Registra um callback para o número de interrupção n.

### I/O de Portas

```c
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
```
Escreve/lê um byte para/de uma porta de I/O.

```c
void outw(uint16_t port, uint16_t val);
uint16_t inw(uint16_t port);
```
Escreve/lê uma word para/de uma porta de I/O.

```c
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);
```
Escreve/lê uma dword para/de uma porta de I/O.

```c
void io_wait(void);
```
Breve atraso para operações de I/O.

## Gerenciamento de Processos

### Controle de Processos

```c
void process_init(void);
```
Inicializa o sistema de gerenciamento de processos.

```c
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority);
```
Cria um novo processo com o ponto de entrada e a prioridade fornecidos. Retorna um ponteiro para o processo ou NULL.

```c
void process_terminate(process_t* process);
```
Encerra e faz a limpeza de um processo.

```c
process_t* process_get_current(void);
```
Obtém o processo em execução no momento.

```c
process_t* process_find_by_pid(uint32_t pid);
```
Localiza um processo pelo ID. Retorna um ponteiro para o processo ou NULL.

### Escalonamento

```c
void scheduler_init(void);
```
Inicializa o scheduler.

```c
void scheduler_add_process(process_t* process);
```
Adiciona um processo à fila de prontos.

```c
void scheduler_remove_process(process_t* process);
```
Remove um processo da fila.

```c
void schedule(void);
```
Invoca o scheduler para selecionar o próximo processo.

```c
void yield(void);
```
Cede voluntariamente a CPU.

## Drivers de Dispositivos

### Timer

```c
void timer_init(uint32_t frequency);
```
Inicializa o timer com a frequência fornecida em Hz.

```c
uint32_t timer_get_ticks(void);
```
Obtém a contagem de ticks desde o boot.

```c
void timer_wait(uint32_t ticks);
```
Aguarda pelo número especificado de ticks.

### Teclado

```c
void keyboard_init(void);
```
Inicializa o driver de teclado.

```c
char keyboard_getchar(void);
```
Lê um caractere do teclado (bloqueante).

```c
int keyboard_available(void);
```
Verifica se há caracteres disponíveis. Retorna valor diferente de zero se houver dados prontos.

```c
void keyboard_flush(void);
```
Limpa o buffer de entrada.

```c
keyboard_state_t keyboard_get_state(void);
```
Obtém o estado atual das teclas modificadoras.

### Mouse

```c
void mouse_init(void);
```
Inicializa o driver de mouse.

```c
mouse_state_t mouse_get_state(void);
```
Obtém a posição atual do mouse e o estado dos botões.

```c
void mouse_set_callback(void (*callback)(int32_t dx, int32_t dy, uint8_t buttons));
```
Registra um callback para eventos de mouse.

### Porta Serial

```c
int serial_init(uint16_t port);
```
Inicializa a porta serial (COM1/COM2/etc). Retorna 0 em caso de sucesso.

```c
void serial_putchar(uint16_t port, char c);
```
Escreve um caractere na porta serial.

```c
void serial_writestring(uint16_t port, const char* str);
```
Escreve uma string na porta serial.

```c
char serial_getchar(uint16_t port);
```
Lê um caractere da porta serial (bloqueante).

```c
int serial_received(uint16_t port);
```
Verifica se há dados disponíveis. Retorna valor diferente de zero se estiver pronto.

### Disco

```c
void disk_init(void);
```
Inicializa o controlador de disco.

```c
int disk_read_sector(uint32_t lba, uint8_t* buffer);
```
Lê um setor de 512 bytes do disco. Retorna 0 em caso de sucesso.

```c
int disk_write_sector(uint32_t lba, const uint8_t* buffer);
```
Escreve um setor de 512 bytes no disco. Retorna 0 em caso de sucesso.

```c
void disk_identify(void);
```
Consulta informações do disco.

## Funções do Terminal

### Display

```c
void terminal_clear(void);
```
Limpa a tela e reposiciona o cursor.

```c
void terminal_setcolor(uint8_t color);
```
Define a cor do texto para a saída subsequente.

```c
void terminal_putchar(char c);
```
Exibe um único caractere.

```c
void terminal_writestring(const char* data);
```
Exibe uma string terminada em nulo.

### Utilitários de Cor

```c
uint8_t vga_entry_color(uint8_t fg, uint8_t bg);
```
Combina as cores de primeiro plano e de fundo.

Cores disponíveis:
- COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN
- COLOR_RED, COLOR_MAGENTA, COLOR_BROWN, COLOR_LIGHT_GREY
- COLOR_DARK_GREY, COLOR_LIGHT_BLUE, COLOR_LIGHT_GREEN, COLOR_LIGHT_CYAN
- COLOR_LIGHT_RED, COLOR_LIGHT_MAGENTA, COLOR_LIGHT_BROWN, COLOR_WHITE

## Funções de String

```c
size_t strlen(const char* str);
```
Calcula o comprimento de uma string.

## Estruturas de Dados

### Bloco de Controle de Processo

```c
typedef struct process {
    uint32_t pid;
    char name[32];
    process_state_t state;
    process_priority_t priority;
    cpu_context_t context;
    page_directory_t* page_dir;
    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t quantum;
    uint32_t total_time;
    struct process* next;
    struct process* parent;
} process_t;
```

### Contexto de CPU

```c
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;
} cpu_context_t;
```

### Estado dos Registradores

```c
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};
```

### Estado do Mouse

```c
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
} mouse_state_t;
```

### Estado do Teclado

```c
typedef struct {
    uint8_t shift : 1;
    uint8_t ctrl : 1;
    uint8_t alt : 1;
    uint8_t caps : 1;
    uint8_t num : 1;
    uint8_t scroll : 1;
} keyboard_state_t;
```

## Enumerações

### Estado do Processo

```c
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;
```

### Prioridade do Processo

```c
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} process_priority_t;
```

## Constantes

### Memória

```c
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
```

### Teclado

```c
#define KEYBOARD_BUFFER_SIZE 256
#define KEY_ESC 27
#define KEY_BACKSPACE '\b'
#define KEY_TAB '\t'
#define KEY_ENTER '\n'
```

### Portas Seriais

```c
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
```

### Disco

```c
#define SECTOR_SIZE 512
```

---

**Nota**: Todas as funções do kernel executam em modo privilegiado com acesso total ao hardware. O uso incorreto pode corromper a memória ou travar o sistema. Sempre valide os parâmetros e trate os erros de forma apropriada.
