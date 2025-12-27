# Interrupt Handling System

## Fundamentals

Interrupts are the primary mechanism for hardware communication and exception handling in x86 systems. They allow external devices to signal the CPU and enable the processor to handle error conditions.

## Interrupt Descriptor Table (IDT)

### Structure

The IDT is an array of 256 entries, each describing how to handle a specific interrupt. Each entry contains:

**Handler Address**: 32-bit pointer to the interrupt service routine. The address is split into low and high 16-bit values for x86 compatibility.

**Segment Selector**: Specifies which code segment contains the handler. Typically points to the kernel code segment.

**Flags**: Control attributes including:
- Descriptor type (interrupt gate, trap gate, task gate)
- Privilege level required to invoke via INT instruction
- Present bit indicating validity

### Interrupt Types

The 256 interrupt vectors are allocated as follows:

**CPU Exceptions (0-31)**: Reserved by Intel for processor-generated exceptions. These include division errors, page faults, general protection faults, and other hardware-detected error conditions.

**Hardware IRQs (32-47)**: Remapped hardware interrupts from the Programmable Interrupt Controller. Originally IRQs 0-15 conflict with CPU exceptions, so we remap them to 32-47.

**Software Interrupts (48-255)**: Available for system calls and custom use. Not currently utilized but reserved for future expansion.

### Initialization

IDT initialization proceeds as follows:

1. Allocate memory for 256 IDT entries
2. Clear all entries to ensure deterministic behavior
3. Configure Programmable Interrupt Controller to remap IRQs
4. Install exception handlers for all CPU exceptions (0-31)
5. Install IRQ handlers for hardware interrupts (32-47)
6. Load IDT address into IDTR register using LIDT instruction

Once loaded, the CPU consults the IDT whenever an interrupt occurs.

## Programmable Interrupt Controller (PIC)

### Purpose

The 8259 PIC manages hardware interrupt requests from peripheral devices. Two PICs are cascaded to provide 15 usable IRQ lines (IRQ2 is used for cascading).

### Remapping

By default, the PIC maps IRQs 0-15 to interrupt vectors 8-15, which conflicts with CPU exceptions. We remap them to 32-47:

1. Send initialization command to both PICs
2. Set master PIC offset to 32
3. Set slave PIC offset to 40
4. Configure cascade connection on IRQ2
5. Set operational mode to 8086/8088 compatible

This remapping is essential for distinguishing hardware interrupts from CPU exceptions.

### End of Interrupt (EOI)

After servicing a hardware interrupt, the handler must send an EOI signal to acknowledge completion:

- If IRQ 0-7: Send EOI to master PIC (port 0x20)
- If IRQ 8-15: Send EOI to both slave (0xA0) and master (0x20)

Failure to send EOI prevents the PIC from delivering further interrupts.

## Exception Handlers

### CPU Exceptions

Each CPU exception has specific semantics:

**Division Error (0)**: Division by zero or quotient too large for destination. Handler can log error and terminate the process.

**Debug (1)**: Breakpoint or single-step for debugging. Future debugger support will utilize this.

**Non-Maskable Interrupt (2)**: Critical hardware failure. Cannot be disabled and indicates serious problem.

**Breakpoint (3)**: INT3 instruction for software debugging. Debuggers insert this to pause execution.

**Overflow (4)**: INTO instruction detected overflow. Rarely used in modern code.

**Bound Range Exceeded (5)**: BOUND instruction detected out of range index.

**Invalid Opcode (6)**: Instruction decoder encountered unknown opcode. Indicates corrupt code or wrong architecture.

**Device Not Available (7)**: Attempt to use FPU when not available. Handler can enable FPU or emulate instruction.

**Double Fault (8)**: Exception occurred while handling another exception. Indicates serious kernel bug. Pushes error code.

**Coprocessor Segment Overrun (9)**: Legacy FPU error, not used on modern processors.

**Invalid TSS (10)**: Task state segment invalid during task switch. Pushes error code.

**Segment Not Present (11)**: Segment descriptor marked as not present. Pushes error code.

**Stack Segment Fault (12)**: Stack segment exceeded limit or not present. Pushes error code.

**General Protection Fault (13)**: General protection violation, most common exception. Indicates privilege violation, segment error, or null pointer. Pushes error code.

**Page Fault (14)**: Virtual memory access to non-present or protected page. Handler can load page from disk or terminate process. Pushes error code. CR2 contains faulting address.

**x87 FPU Error (16)**: Floating point exception like invalid operation, division by zero, or overflow.

**Alignment Check (17)**: Misaligned memory access with alignment checking enabled. Pushes error code.

**Machine Check (18)**: Hardware error detection. Indicates failing hardware.

**SIMD Floating Point (19)**: SSE floating point exception.

### Error Codes

Some exceptions push an error code onto the stack providing additional context. The error code format depends on the exception type.

For page faults, the error code indicates:
- Bit 0: Present (1 = protection violation, 0 = page not present)
- Bit 1: Write (1 = write access, 0 = read access)
- Bit 2: User (1 = user mode, 0 = kernel mode)
- Bit 3: Reserved bit violation
- Bit 4: Instruction fetch

This detailed information allows the page fault handler to respond appropriately.

### Handler Implementation

Exception handlers follow a standard pattern:

1. Entry stub (assembly) saves CPU state
2. Pushes interrupt number and error code
3. Calls common handler dispatcher
4. Dispatcher invokes registered C handler
5. C handler performs exception-specific processing
6. Returns to dispatcher
7. Dispatcher restores CPU state
8. IRET instruction returns to interrupted code

This layered approach separates architecture-specific assembly from portable C code.

## IRQ Handlers

### Hardware Interrupts

Hardware devices signal the CPU via IRQ lines. Common IRQs include:

**IRQ 0 (Timer)**: Programmable Interval Timer generates periodic interrupts for timekeeping and scheduling.

**IRQ 1 (Keyboard)**: PS/2 keyboard controller signals when data is available.

**IRQ 2 (Cascade)**: Connects slave PIC to master, not available for devices.

**IRQ 3/4 (Serial)**: COM2 and COM1 serial ports for communication.

**IRQ 6 (Floppy)**: Floppy disk controller, rarely used on modern systems.

**IRQ 12 (Mouse)**: PS/2 mouse controller signals movement or button events.

**IRQ 14/15 (IDE)**: Primary and secondary IDE controllers for hard disks.

### Handler Registration

Device drivers register handlers for their IRQs:

```c
register_interrupt_handler(33, keyboard_handler);
```

The interrupt number is IRQ number + 32 (due to remapping).

When the interrupt fires, the dispatcher looks up and invokes the registered handler.

### Handler Requirements

IRQ handlers must:
- Execute quickly to minimize interrupt latency
- Not block or sleep
- Be reentrant if the same IRQ can nest
- Send EOI before returning
- Save/restore any registers they modify

Long operations should be deferred to a process or bottom half handler.

## Assembly Stubs

### Purpose

Each interrupt needs an assembly stub that:
- Saves CPU state
- Aligns the stack consistently
- Calls the C handler
- Restores CPU state
- Returns from interrupt

### Stub Generation

Rather than writing 256 individual stubs, macros generate them:

```asm
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push byte 0        ; Dummy error code
        push byte %1       ; Interrupt number
        jmp isr_common_stub
%endmacro
```

This macro expands to create stub functions for interrupts that don't push error codes.

Similar macros handle interrupts with error codes and IRQs.

### Common Stub

All stubs jump to a common routine that:

1. Saves all general purpose registers (PUSHA)
2. Saves segment registers
3. Loads kernel data segment
4. Calls C dispatcher
5. Restores segment registers
6. Restores general purpose registers (POPA)
7. Cleans up pushed error code and interrupt number
8. Executes IRET to return

This common path ensures consistency and reduces code duplication.

## Interrupt Dispatching

### Dispatcher Function

The C dispatcher receives a pointer to the saved register structure:

```c
void isr_handler(struct registers* regs) {
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    } else {
        // Unhandled interrupt
        print_error(regs->int_no);
    }
}
```

This allows multiple subsystems to handle their own interrupts through registered callbacks.

### IRQ Dispatcher

IRQ handling is similar but includes EOI handling:

```c
void irq_handler(struct registers* regs) {
    // Send EOI to PIC
    if (regs->int_no >= 40) {
        outb(0xA0, 0x20);  // Slave PIC
    }
    outb(0x20, 0x20);      // Master PIC
    
    // Invoke registered handler
    if (interrupt_handlers[regs->int_no] != 0) {
        isr_t handler = interrupt_handlers[regs->int_no];
        handler(regs);
    }
}
```

The EOI must be sent before calling the handler in case the handler takes significant time.

## Interrupt Safety

### Critical Sections

Some kernel code cannot safely be interrupted. Such code disables interrupts:

```c
__asm__ volatile("cli");  // Clear interrupt flag
// ... critical code ...
__asm__ volatile("sti");  // Set interrupt flag
```

Interrupts should be disabled for the minimum time necessary to avoid degrading system responsiveness.

### Reentrancy

Interrupt handlers can be interrupted by higher priority interrupts. Handlers must be reentrant, using only local variables or properly synchronized global state.

## Performance Considerations

### Latency

Interrupt latency (time from hardware signal to handler execution) should be minimized:
- Keep critical sections short
- Use efficient handler registration lookup
- Optimize assembly stubs
- Defer long operations to process context

### Throughput

High interrupt rates can saturate the CPU:
- Batch process multiple events when possible
- Use interrupt coalescing on high-speed devices
- Consider polling for extremely high-rate devices

## Future Enhancements

Several improvements are planned:

**Advanced PIC**: Replace PIC with APIC for better multicore support

**MSI/MSI-X**: Modern interrupt delivery mechanism for PCI Express devices

**Interrupt Threading**: Run interrupt handlers as high-priority threads for better scheduling control

**Deferred Processing**: Formal bottom-half mechanism for deferring work out of interrupt context

---

**Implementation Files**:
- `kernel/interrupts/idt.c` - IDT management and initialization
- `kernel/interrupts/idt.h` - IDT structures and prototypes
- `kernel/interrupts/interrupt.asm` - Assembly interrupt stubs
- `kernel/interrupts/io.h` - Port I/O operations
