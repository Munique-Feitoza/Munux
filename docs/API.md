# Munux Kernel API Reference

## Memory Management

### Physical Memory Manager

```c
void pmm_init(uint32_t mem_size);
```
Initialize physical memory manager with total system RAM.

```c
uint32_t pmm_alloc_frame(void);
```
Allocate a 4KB physical frame. Returns physical address or 0 if out of memory.

```c
void pmm_free_frame(uint32_t frame_addr);
```
Free a previously allocated frame.

```c
uint32_t pmm_get_total_memory(void);
```
Returns total system memory in bytes.

```c
uint32_t pmm_get_free_memory(void);
```
Returns available memory in bytes.

### Virtual Memory Manager

```c
void vmm_init(void);
```
Initialize virtual memory with paging enabled.

```c
void vmm_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);
```
Map virtual page to physical frame with specified flags:
- PAGE_PRESENT: Page is present in memory
- PAGE_WRITE: Page is writable
- PAGE_USER: Page accessible from user mode

```c
void vmm_unmap_page(uint32_t virtual_addr);
```
Remove mapping for virtual page.

```c
uint32_t vmm_get_physical_address(uint32_t virtual_addr);
```
Translate virtual address to physical address. Returns 0 if not mapped.

```c
page_directory_t* vmm_get_current_directory(void);
```
Get current page directory.

```c
void vmm_switch_directory(page_directory_t* dir);
```
Switch to different page directory.

### Heap Allocator

```c
void heap_init(void);
```
Initialize kernel heap.

```c
void* kmalloc(size_t size);
```
Allocate memory from kernel heap. Returns pointer or NULL if allocation fails.

```c
void* kmalloc_aligned(size_t size);
```
Allocate page-aligned memory.

```c
void* kmalloc_physical(size_t size, uint32_t* phys_addr);
```
Allocate memory and return both virtual and physical addresses.

```c
void kfree(void* ptr);
```
Free previously allocated memory.

### Memory Utilities

```c
void* memset(void* dest, int val, size_t len);
```
Fill memory region with byte value.

```c
void* memcpy(void* dest, const void* src, size_t len);
```
Copy memory from source to destination.

```c
int memcmp(const void* s1, const void* s2, size_t n);
```
Compare two memory regions. Returns 0 if equal, <0 or >0 otherwise.

## Interrupt Handling

### IDT Management

```c
void idt_init(void);
```
Initialize Interrupt Descriptor Table and reprogram PIC.

```c
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
```
Configure specific IDT entry.

```c
void register_interrupt_handler(uint8_t n, isr_t handler);
```
Register callback for interrupt number n.

### Port I/O

```c
void outb(uint16_t port, uint8_t val);
uint8_t inb(uint16_t port);
```
Write/read byte to/from I/O port.

```c
void outw(uint16_t port, uint16_t val);
uint16_t inw(uint16_t port);
```
Write/read word to/from I/O port.

```c
void outl(uint16_t port, uint32_t val);
uint32_t inl(uint16_t port);
```
Write/read dword to/from I/O port.

```c
void io_wait(void);
```
Brief delay for I/O operations.

## Process Management

### Process Control

```c
void process_init(void);
```
Initialize process management system.

```c
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority);
```
Create new process with given entry point and priority. Returns process pointer or NULL.

```c
void process_terminate(process_t* process);
```
Terminate and clean up process.

```c
process_t* process_get_current(void);
```
Get currently running process.

```c
process_t* process_find_by_pid(uint32_t pid);
```
Find process by ID. Returns process pointer or NULL.

### Scheduling

```c
void scheduler_init(void);
```
Initialize scheduler.

```c
void scheduler_add_process(process_t* process);
```
Add process to ready queue.

```c
void scheduler_remove_process(process_t* process);
```
Remove process from queue.

```c
void schedule(void);
```
Invoke scheduler to select next process.

```c
void yield(void);
```
Voluntarily give up CPU.

## Device Drivers

### Timer

```c
void timer_init(uint32_t frequency);
```
Initialize timer with given frequency in Hz.

```c
uint32_t timer_get_ticks(void);
```
Get tick count since boot.

```c
void timer_wait(uint32_t ticks);
```
Sleep for specified number of ticks.

### Keyboard

```c
void keyboard_init(void);
```
Initialize keyboard driver.

```c
char keyboard_getchar(void);
```
Read character from keyboard (blocking).

```c
int keyboard_available(void);
```
Check if characters are available. Returns non-zero if data ready.

```c
void keyboard_flush(void);
```
Clear input buffer.

```c
keyboard_state_t keyboard_get_state(void);
```
Get current modifier key states.

### Mouse

```c
void mouse_init(void);
```
Initialize mouse driver.

```c
mouse_state_t mouse_get_state(void);
```
Get current mouse position and button states.

```c
void mouse_set_callback(void (*callback)(int32_t dx, int32_t dy, uint8_t buttons));
```
Register callback for mouse events.

### Serial Port

```c
int serial_init(uint16_t port);
```
Initialize serial port (COM1/COM2/etc). Returns 0 on success.

```c
void serial_putchar(uint16_t port, char c);
```
Write character to serial port.

```c
void serial_writestring(uint16_t port, const char* str);
```
Write string to serial port.

```c
char serial_getchar(uint16_t port);
```
Read character from serial port (blocking).

```c
int serial_received(uint16_t port);
```
Check if data is available. Returns non-zero if ready.

### Disk

```c
void disk_init(void);
```
Initialize disk controller.

```c
int disk_read_sector(uint32_t lba, uint8_t* buffer);
```
Read 512-byte sector from disk. Returns 0 on success.

```c
int disk_write_sector(uint32_t lba, const uint8_t* buffer);
```
Write 512-byte sector to disk. Returns 0 on success.

```c
void disk_identify(void);
```
Query disk information.

## Terminal Functions

### Display

```c
void terminal_clear(void);
```
Clear screen and reset cursor.

```c
void terminal_setcolor(uint8_t color);
```
Set text color for subsequent output.

```c
void terminal_putchar(char c);
```
Display single character.

```c
void terminal_writestring(const char* data);
```
Display null-terminated string.

### Color Utilities

```c
uint8_t vga_entry_color(uint8_t fg, uint8_t bg);
```
Combine foreground and background colors.

Available colors:
- COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN
- COLOR_RED, COLOR_MAGENTA, COLOR_BROWN, COLOR_LIGHT_GREY
- COLOR_DARK_GREY, COLOR_LIGHT_BLUE, COLOR_LIGHT_GREEN, COLOR_LIGHT_CYAN
- COLOR_LIGHT_RED, COLOR_LIGHT_MAGENTA, COLOR_LIGHT_BROWN, COLOR_WHITE

## String Functions

```c
size_t strlen(const char* str);
```
Calculate string length.

## Data Structures

### Process Control Block

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

### CPU Context

```c
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;
} cpu_context_t;
```

### Register State

```c
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};
```

### Mouse State

```c
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;
} mouse_state_t;
```

### Keyboard State

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

## Enumerations

### Process State

```c
typedef enum {
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;
```

### Process Priority

```c
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} process_priority_t;
```

## Constants

### Memory

```c
#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4
```

### Keyboard

```c
#define KEYBOARD_BUFFER_SIZE 256
#define KEY_ESC 27
#define KEY_BACKSPACE '\b'
#define KEY_TAB '\t'
#define KEY_ENTER '\n'
```

### Serial Ports

```c
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8
```

### Disk

```c
#define SECTOR_SIZE 512
```

---

**Note**: All kernel functions run in privileged mode with full hardware access. Incorrect use can corrupt memory or crash the system. Always validate parameters and handle errors appropriately.
