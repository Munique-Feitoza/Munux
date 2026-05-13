# Munux Documentation Index

Welcome to the Munux operating system documentation. This comprehensive guide covers every aspect of the kernel implementation, from high-level architecture to low-level implementation details.

> 📐 **UML diagrams**: Every core subsystem document now ships with Mermaid-based UML diagrams (component, class, sequence, state and activity). They render natively on GitHub — no tooling required. If you are reading locally, any Markdown viewer that supports Mermaid (VS Code with the Markdown Preview Mermaid extension, Obsidian, Typora, etc.) will display them.

## Getting Started

New to Munux? Start here:

1. **[README.md](../README.md)** — Project overview and vision
2. **[BUILD.md](BUILD.md)** — How to build and run Munux (C, Assembly and Rust)
3. **[ARCHITECTURE.md](ARCHITECTURE.md)** — High-level system design
4. **[RUST.md](RUST.md)** — Rust integration strategy and FFI boundary

## Core Documentation

### System Architecture

**[ARCHITECTURE.md](ARCHITECTURE.md)**  
Comprehensive overview of Munux's design philosophy, component organization, and system architecture. Understand how all pieces fit together.

**[ROADMAP.md](ROADMAP.md)**  
Development timeline showing completed features, current work, and future plans. See where Munux is heading.

**[RUST.md](RUST.md)**  
Strategy for the Rust adoption (v0.3): toolchain, custom target specification, FFI boundary, panic handling, allocator contract, and the order in which subsystems will be ported from C to Rust.

### Subsystems

**[MEMORY.md](MEMORY.md)**  
Deep dive into memory management: physical frame allocation, virtual memory paging, and heap allocation. Learn how Munux manages one of the most critical resources.

**[PROCESSES.md](PROCESSES.md)**  
Process management and scheduling internals. Understand how Munux achieves multitasking through process control blocks, scheduling algorithms, and context switching.

**[INTERRUPTS.md](INTERRUPTS.md)**  
The interrupt handling system forms the foundation of hardware communication. Learn about the IDT, exception handlers, IRQ management, and the PIC.

**[DRIVERS.md](DRIVERS.md)**  
Device driver architecture and implementations. Covers timer, keyboard, mouse, serial port, and disk drivers with detailed protocol explanations.

### Development Resources

**[BUILD.md](BUILD.md)**  
Complete guide to building, running, testing, and debugging Munux. Includes toolchain setup, Makefile walkthrough, and troubleshooting tips.

**[API.md](API.md)**  
Comprehensive kernel API reference. Every function, structure, and constant documented with usage examples.

## UML Diagram Map

Quick jump-list of the diagrams added across the documentation:

| Document | Diagrams |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Component diagram (kernel ↔ hardware, with Rust layer) · Boot sequence |
| [MEMORY.md](MEMORY.md) | Class diagram (PMM/VMM/Heap) · Heap-expansion sequence · Address-translation activity |
| [PROCESSES.md](PROCESSES.md) | Class diagram (PCB/Scheduler) · Process state machine · Context-switch sequence |
| [INTERRUPTS.md](INTERRUPTS.md) | IDT vector layout · Interrupt lifecycle sequence · Exception/IRQ dispatch activity |
| [DRIVERS.md](DRIVERS.md) | Driver component diagram · Keyboard IRQ sequence |
| [RUST.md](RUST.md) | FFI boundary diagram · Build pipeline sequence · Subsystem port progression |
| [README.md](../README.md) | Kernel-at-a-glance overview diagram (with FFI edges) |

## Documentation by Topic

### For Learners

If you're learning operating systems:

1. Start with [ARCHITECTURE.md](ARCHITECTURE.md) for the big picture
2. Read [INTERRUPTS.md](INTERRUPTS.md) to understand hardware communication
3. Study [MEMORY.md](MEMORY.md) for memory management concepts
4. Explore [PROCESSES.md](PROCESSES.md) to see multitasking in action
5. Review [DRIVERS.md](DRIVERS.md) for real hardware interaction examples

### For Developers

If you're contributing to Munux:

1. Review [BUILD.md](BUILD.md) to set up your environment
2. Consult [API.md](API.md) for function signatures and usage
3. Check [ROADMAP.md](ROADMAP.md) to find areas needing work
4. Study relevant subsystem docs ([MEMORY.md](MEMORY.md), [PROCESSES.md](PROCESSES.md), etc.)
5. Follow coding standards shown in existing code

### For System Architects

If you're studying OS design:

1. [ARCHITECTURE.md](ARCHITECTURE.md) explains design decisions
2. [MEMORY.md](MEMORY.md) shows three-tier memory management
3. [PROCESSES.md](PROCESSES.md) details scheduling algorithms
4. [INTERRUPTS.md](INTERRUPTS.md) covers interrupt architecture
5. [ROADMAP.md](ROADMAP.md) outlines future architectural plans

## Quick Reference

### Common Tasks

**Building Munux**
```bash
cd munux-core
make
```
See [BUILD.md](BUILD.md) for details.

**Running in QEMU**
```bash
make run
```

**Debugging with GDB**
```bash
make debug
# In another terminal:
gdb build/kernel.elf
(gdb) target remote localhost:1234
```

**Finding API documentation**
See [API.md](API.md) - organized by subsystem with complete signatures.

### Key Concepts

**Memory Management**
- Physical: Bitmap allocation of 4KB frames
- Virtual: Two-level paging (PD + PT)
- Heap: First-fit allocator with coalescing

**Process Scheduling**
- Algorithm: Round-robin with priorities
- Preemption: Quantum-based via timer interrupt
- Context Switch: Assembly routine saving/restoring state

**Interrupt Handling**
- Exceptions: CPU-generated (divide by zero, page fault, etc.)
- IRQs: Hardware interrupts (timer, keyboard, etc.)
- Flow: IDT → Assembly stub → C handler

## Documentation Standards

All Munux documentation follows these principles:

**Comprehensive**: Cover all aspects of the topic, from overview to implementation details

**Accurate**: Reflect current implementation, update when code changes

**Accessible**: Written for readers with varying experience levels

**Practical**: Include concrete examples and usage patterns

**Well-Organized**: Logical structure with clear hierarchy

**Cross-Referenced**: Link to related documentation

## Contributing to Documentation

Documentation improvements are always welcome:

- Fix typos or unclear explanations
- Add missing details or examples
- Update outdated information
- Create new guides or tutorials
- Improve organization or navigation

## Documentation Structure

```
docs/
├── INDEX.md           # This file — documentation guide
├── ARCHITECTURE.md    # System architecture overview
├── RUST.md            # Rust integration strategy and FFI boundary
├── MEMORY.md          # Memory management subsystem
├── PROCESSES.md       # Process management and scheduling
├── INTERRUPTS.md      # Interrupt handling system
├── DRIVERS.md         # Device drivers
├── BUILD.md           # Building and running Munux
├── API.md             # Complete API reference
├── STRUCTURE.md       # Repository layout reference
└── ROADMAP.md         # Development roadmap
```

## Additional Resources

### Source Code

The most accurate documentation is the code itself:

```
munux-core/
├── kernel/           # Main kernel code
│   ├── interrupts/  # Interrupt handling (C + Assembly)
│   ├── memory/      # Memory management (C; heap migrating to Rust)
│   ├── process/     # Process management (C + Assembly)
│   ├── drivers/     # Device drivers (C)
│   └── rust/        # Rust no_std static library (v0.3+)
├── boot/            # Bootloader (Assembly)
└── Makefile         # Build system (orchestrates GCC, NASM, Cargo, LD)
```

### External Resources

Understanding x86 architecture and OS concepts:

- Intel 64 and IA-32 Architectures Software Developer Manuals
- OSDev Wiki (osdev.org)
- "Operating Systems: Three Easy Pieces" by Remzi Arpaci-Dusseau
- "Modern Operating Systems" by Andrew Tanenbaum
- "The Design and Implementation of the FreeBSD Operating System"

## Getting Help

Stuck or confused? Try:

1. Search this documentation using your editor's search
2. Read the relevant subsystem documentation in detail
3. Examine the source code implementation
4. Check external resources for background concepts
5. Open an issue on the repository

## Feedback

Documentation feedback is valuable:

- What's unclear or confusing?
- What's missing?
- What examples would help?
- How can organization improve?

Your input helps make Munux documentation better for everyone.

---

**Documentation Version**: 0.3  
**Last Updated**: Corresponds to kernel version 0.3 (Rust Adoption)  
**Maintained by**: Munique Feitoza  
**License**: GPLv3
