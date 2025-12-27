# Munux Project Structure

## Repository Organization

The Munux repository is organized to separate concerns and maintain clarity:

```
Munux/
├── AUTHORSHIP.md              # Author declaration and credits
├── LICENSE                    # GPLv3 license text
├── README.md                  # Project overview and vision
├── docs/                      # Comprehensive documentation
│   ├── INDEX.md              # Documentation navigation guide
│   ├── ARCHITECTURE.md       # System architecture overview
│   ├── MEMORY.md            # Memory management details
│   ├── PROCESSES.md         # Process management and scheduling
│   ├── INTERRUPTS.md        # Interrupt handling system
│   ├── DRIVERS.md           # Device driver architecture
│   ├── BUILD.md             # Build and testing guide
│   ├── API.md               # Complete API reference
│   └── ROADMAP.md           # Development roadmap
└── munux-core/               # Kernel implementation
    ├── README.md             # Core kernel documentation
    ├── Makefile              # Build system
    ├── boot/                 # Bootloader code
    │   ├── bootloader.asm   # Main bootloader
    │   └── pmode.asm        # Protected mode transition
    ├── kernel/               # Kernel source code
    │   ├── kernel.c         # Main kernel initialization
    │   ├── kernel.h         # Kernel header and types
    │   ├── kernel.ld        # Linker script
    │   ├── interrupts/      # Interrupt handling subsystem
    │   │   ├── idt.c       # IDT implementation
    │   │   ├── idt.h       # IDT header
    │   │   ├── interrupt.asm  # Assembly interrupt stubs
    │   │   └── io.h        # Port I/O operations
    │   ├── memory/          # Memory management subsystem
    │   │   ├── memory.h    # Memory subsystem header
    │   │   ├── pmm.c       # Physical memory manager
    │   │   ├── vmm.c       # Virtual memory manager
    │   │   ├── heap.c      # Heap allocator
    │   │   └── utils.c     # Memory utilities
    │   ├── process/         # Process management subsystem
    │   │   ├── process.h   # Process subsystem header
    │   │   ├── process.c   # Process management
    │   │   ├── scheduler.c # Scheduler implementation
    │   │   └── switch.asm  # Context switching
    │   └── drivers/         # Device drivers
    │       ├── timer.c/.h  # Timer driver
    │       ├── keyboard.c/.h # Keyboard driver
    │       ├── mouse.c/.h   # Mouse driver
    │       ├── serial.c/.h  # Serial port driver
    │       └── disk.c/.h    # Disk driver
    └── build/               # Build outputs (generated)
        ├── *.o              # Object files
        ├── kernel.elf       # Kernel ELF executable
        ├── kernel.bin       # Kernel flat binary
        ├── bootloader.bin   # Bootloader binary
        └── munux.iso        # Bootable ISO image
```

## Directory Purposes

### Root Directory

**AUTHORSHIP.md**: Declares project authorship by Munique Feitoza with timestamp and licensing information.

**LICENSE**: Full text of the GNU General Public License v3.0 under which Munux is distributed.

**README.md**: High-level project overview covering vision, goals, philosophy, and planned features.

### docs/

Contains all project documentation written in Markdown format. Each document covers a specific aspect of the system in depth.

Documentation is written for multiple audiences:
- Learners seeking to understand OS concepts
- Developers contributing to Munux
- System architects studying design decisions

All documentation cross-references related documents and source code.

### munux-core/

The kernel implementation itself. All source code for the bootloader and kernel resides here.

Organized by subsystem to maintain clear separation of concerns and enable independent development of each component.

### munux-core/boot/

Bootloader code written in x86 assembly. Responsible for:
- Loading kernel from disk into memory
- Transitioning from 16-bit real mode to 32-bit protected mode
- Setting up initial GDT
- Transferring control to kernel

**bootloader.asm**: Main bootloader logic including disk I/O and kernel loading

**pmode.asm**: Protected mode initialization code (currently integrated into bootloader.asm)

### munux-core/kernel/

Main kernel source code in C and assembly. Contains:

**kernel.c/h**: Entry point and initialization sequence for all subsystems

**kernel.ld**: Linker script controlling memory layout of the kernel binary

### munux-core/kernel/interrupts/

Interrupt handling subsystem. Manages all CPU exceptions and hardware interrupts.

**idt.c/h**: Interrupt Descriptor Table management and initialization

**interrupt.asm**: Assembly stubs for each interrupt vector (macros generate ISR/IRQ handlers)

**io.h**: Inline functions for port I/O operations (inb, outb, inw, outw, etc.)

### munux-core/kernel/memory/

Memory management subsystem implementing three-tier memory abstraction.

**memory.h**: Public API for all memory functions

**pmm.c**: Physical Memory Manager - frame allocation via bitmap

**vmm.c**: Virtual Memory Manager - paging with page directory and page tables

**heap.c**: Heap allocator - malloc/free implementation using first-fit algorithm

**utils.c**: Memory manipulation utilities (memset, memcpy, memcmp)

### munux-core/kernel/process/

Process management and scheduling subsystem.

**process.h**: Public API for process operations

**process.c**: Process creation, termination, and management

**scheduler.c**: Round-robin scheduler with priority queues

**switch.asm**: Context switching in assembly to save/restore CPU state

### munux-core/kernel/drivers/

Device drivers for essential hardware.

Each driver consists of .c implementation and .h header:

**timer**: Programmable Interval Timer for timekeeping and scheduling

**keyboard**: PS/2 keyboard with scancode translation and input buffering

**mouse**: PS/2 mouse with movement tracking and button states

**serial**: RS-232 serial port for debugging and external communication

**disk**: ATA/IDE disk controller for mass storage access

### munux-core/build/

Generated during compilation. Contains intermediate and final build outputs.

**NOT** checked into version control - recreated on each build.

**.o files**: Compiled object files for each .c and .asm source

**kernel.elf**: Linked kernel in ELF format with debug symbols

**kernel.bin**: Flat binary kernel extracted from ELF for loading

**bootloader.bin**: Assembled bootloader (exactly 512 bytes with 0xAA55 signature)

**munux.iso**: Bootable ISO image combining bootloader and kernel

## File Naming Conventions

**Assembly files**: .asm extension (NASM syntax)

**C source**: .c extension

**C headers**: .h extension

**Markdown docs**: .md extension

**Build scripts**: Makefile (no extension)

**Linker scripts**: .ld extension

## Code Organization Principles

### Separation of Concerns

Each subsystem is independent with well-defined interfaces. Memory management doesn't need to know about process internals. Drivers don't depend on scheduling details.

### Layered Architecture

Higher layers build on lower layers:
- Layer 0: Hardware (CPU, devices)
- Layer 1: Drivers and interrupt handlers
- Layer 2: Memory and process management
- Layer 3: System services (future: VFS, syscalls)
- Layer 4: User space (future)

### Header Files

Headers declare public APIs and data structures. Implementation details remain in .c files.

Headers use include guards to prevent multiple inclusion:
```c
#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H
// declarations
#endif
```

### Assembly Integration

Assembly code interfaces with C through declared prototypes:

C declares: `extern void switch_to_process(...);`

Assembly defines: `global switch_to_process`

This allows seamless integration while keeping performance-critical code in assembly.

## Build System Organization

### Makefile Structure

Variables section defines tools and flags

Pattern rules compile sources to objects

Explicit rules handle special cases (bootloader, linking)

Phony targets provide user commands (all, clean, run)

### Compilation Phases

1. Assemble bootloader to flat binary
2. Compile C sources to ELF objects
3. Assemble ASM sources to ELF objects
4. Link all objects into kernel ELF
5. Extract flat binary from ELF
6. Combine into bootable ISO

### Dependency Tracking

Make automatically tracks dependencies through header includes. Changing a header recompiles all sources that include it.

## Development Workflow

### Adding New Features

1. Plan the feature and identify affected subsystems
2. Create or modify headers with new APIs
3. Implement functionality in .c or .asm files
4. Update Makefile if adding new source files
5. Test thoroughly in QEMU
6. Update relevant documentation
7. Commit changes with descriptive message

### Modifying Existing Code

1. Understand current implementation (read code and docs)
2. Make targeted changes preserving existing interfaces when possible
3. Update documentation to reflect changes
4. Test to ensure no regressions
5. Commit with explanation of changes

### Debugging Issues

1. Reproduce the problem reliably
2. Add serial debug output to narrow down location
3. Use GDB to inspect state if needed
4. Fix the root cause, not symptoms
5. Add checks to prevent similar issues
6. Document the fix if non-obvious

## Documentation Maintenance

Documentation must stay synchronized with code:

- Update docs when changing behavior
- Add docs for new features
- Remove docs for deleted features
- Keep examples accurate and working

Good documentation is as important as good code for an educational OS.

## Quality Standards

### Code Quality

- Clear, descriptive names for functions and variables
- Comments explaining "why", not "what"
- Consistent indentation and formatting
- No unnecessary complexity
- Handle errors gracefully

### Documentation Quality

- Accurate reflection of current implementation
- Organized logically with clear hierarchy
- Examples that compile and work
- Cross-references to related information
- Appropriate detail level for target audience

### Testing Quality

- Boot and run successfully in QEMU
- No crashes or panics under normal operation
- Proper error handling for abnormal conditions
- Memory not corrupted or leaked
- Performance acceptable for intended use

## Future Organization

As Munux grows, additional directories will be added:

**userspace/**: User mode programs and libraries

**tools/**: Build and development utilities

**tests/**: Automated test suites

**fs/**: File system implementations

**net/**: Networking stack

The current structure provides a solid foundation for this growth while maintaining clarity and organization.

---

**See Also**:
- [BUILD.md](BUILD.md) for build system details
- [ARCHITECTURE.md](ARCHITECTURE.md) for design overview
- Source code for implementation details
