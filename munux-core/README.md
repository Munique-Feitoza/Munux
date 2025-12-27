# Munux Core

This is the core kernel of the Munux operating system - a fully functional, modern OS kernel implementing fundamental concepts including memory management, process scheduling, interrupt handling, and device drivers.

## Features

### Interrupt Management
- Complete IDT (Interrupt Descriptor Table) with 256 entries
- CPU exception handlers for all x86 exceptions
- Hardware IRQ management with PIC reprogramming
- Extensible interrupt handler registration system

### Memory Management
- **Physical Memory Manager**: Bitmap-based frame allocation for 4KB pages
- **Virtual Memory Manager**: Full paging support with page directory and page tables
- **Heap Allocator**: Dynamic memory allocation with malloc/free
- Memory protection and isolation between kernel spaces
- Efficient memory utilization with block coalescing

### Process Management
- Process Control Block (PCB) structure with complete state tracking
- Round-robin scheduler with four priority levels
- Preemptive multitasking with quantum-based time slicing
- Context switching implemented in optimized assembly
- Support for process creation, termination, and state transitions

### Device Drivers
- **Timer (PIT)**: Programmable Interval Timer for scheduling and timekeeping
- **Keyboard**: Complete PS/2 keyboard driver with ABNT2 layout, modifier keys, and circular buffer
- **Mouse**: PS/2 mouse driver with three-button support and movement tracking
- **Serial Port**: RS-232 communication for debugging and external device communication
- **Disk**: ATA/IDE disk controller with sector-level read/write operations

## Building

### Prerequisites

- **i686-elf-gcc**: Cross-compiler for bare-metal x86
- **i686-elf-binutils**: Binary utilities (linker, assembler)
- **NASM**: Netwide Assembler for bootloader and stubs
- **GRUB**: grub-mkrescue for creating bootable ISOs
- **QEMU**: x86 system emulator for testing

### Compilation

```bash
make            # Build everything
make clean      # Remove build artifacts
make run        # Build and run in QEMU
make debug      # Build and start with GDB server
make test       # Run automated tests
```

## Documentation

Comprehensive documentation is available in the `docs/` directory:

- **ARCHITECTURE.md**: System design and component overview
- **MEMORY.md**: Memory management subsystem details
- **PROCESSES.md**: Process management and scheduling
- **INTERRUPTS.md**: Interrupt handling system
- **DRIVERS.md**: Device driver architecture
- **BUILD.md**: Building and testing procedures
- **API.md**: Complete kernel API reference
- **ROADMAP.md**: Development roadmap and future plans

## License

This project is licensed under the GPLv3.

## Author

Munique Feitoza

## Repository

https://github.com/Munique-Feitoza/Munux

