/*
 * Munux Kernel - Interrupt Descriptor Table
 * 
 * Gerenciamento de interrupções e exceções do processador.
 * A IDT é crucial para o funcionamento do sistema, permitindo
 * tratamento de hardware e exceções do processador.
 */

#ifndef IDT_H
#define IDT_H

#include "../kernel.h"

// Número total de entradas na IDT
#define IDT_ENTRIES 256

// Estrutura de uma entrada na IDT (8 bytes)
struct idt_entry {
    uint16_t base_low;      // Bits 0-15 do endereço do handler
    uint16_t selector;      // Seletor de segmento de código
    uint8_t  always0;       // Sempre 0
    uint8_t  flags;         // Flags de tipo e atributos
    uint16_t base_high;     // Bits 16-31 do endereço do handler
} __attribute__((packed));

// Ponteiro para a IDT (usado pela instrução LIDT)
struct idt_ptr {
    uint16_t limit;         // Tamanho da IDT - 1
    uint32_t base;          // Endereço base da IDT
} __attribute__((packed));

// Estrutura de registradores salvos durante interrupção
typedef struct registers {
    uint32_t ds;                                        // Segmento de dados
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;   // Registradores salvos por pusha
    uint32_t int_no, err_code;                          // Número da interrupção e código de erro
    uint32_t eip, cs, eflags, useresp, ss;             // Salvos automaticamente pelo processador
} registers_t;

// Tipo de função para handlers de interrupção
typedef void (*isr_t)(registers_t);

// Funções principais
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void register_interrupt_handler(uint8_t n, isr_t handler);

// ISRs (Interrupt Service Routines) - Exceções do processador
extern void isr0(void);   // Division by zero
extern void isr1(void);   // Debug
extern void isr2(void);   // Non-maskable interrupt
extern void isr3(void);   // Breakpoint
extern void isr4(void);   // Overflow
extern void isr5(void);   // Bound range exceeded
extern void isr6(void);   // Invalid opcode
extern void isr7(void);   // Device not available
extern void isr8(void);   // Double fault
extern void isr9(void);   // Coprocessor segment overrun
extern void isr10(void);  // Invalid TSS
extern void isr11(void);  // Segment not present
extern void isr12(void);  // Stack-segment fault
extern void isr13(void);  // General protection fault
extern void isr14(void);  // Page fault
extern void isr15(void);  // Reserved
extern void isr16(void);  // x87 floating-point exception
extern void isr17(void);  // Alignment check
extern void isr18(void);  // Machine check
extern void isr19(void);  // SIMD floating-point exception
extern void isr20(void);  // Virtualization exception
extern void isr21(void);  // Reserved
extern void isr22(void);  // Reserved
extern void isr23(void);  // Reserved
extern void isr24(void);  // Reserved
extern void isr25(void);  // Reserved
extern void isr26(void);  // Reserved
extern void isr27(void);  // Reserved
extern void isr28(void);  // Reserved
extern void isr29(void);  // Reserved
extern void isr30(void);  // Security exception
extern void isr31(void);  // Reserved

// IRQs (Hardware Interrupts)
extern void irq0(void);   // Timer
extern void irq1(void);   // Keyboard
extern void irq2(void);   // Cascade
extern void irq3(void);   // COM2
extern void irq4(void);   // COM1
extern void irq5(void);   // LPT2
extern void irq6(void);   // Floppy disk
extern void irq7(void);   // LPT1
extern void irq8(void);   // CMOS real-time clock
extern void irq9(void);   // Free
extern void irq10(void);  // Free
extern void irq11(void);  // Free
extern void irq12(void);  // PS2 Mouse
extern void irq13(void);  // FPU
extern void irq14(void);  // Primary ATA
extern void irq15(void);  // Secondary ATA

// Handler comum para ISRs e IRQs
void isr_handler(struct registers* regs);
void irq_handler(struct registers* regs);

#endif // IDT_H
