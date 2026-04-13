# 🌌 Munux Operating System

![Status](https://img.shields.io/badge/Status-Active_Development-brightgreen)
![Kernel](https://img.shields.io/badge/Type-Monolithic_Kernel-blue)
![Arch](https://img.shields.io/badge/Arch-x86-orange)
![License](https://img.shields.io/badge/License-GPLv3-red)

**Munux** is an educational operating system built from scratch with a bold goal: to make low-level systems programming accessible, elegant, and understandable. It is designed for those who don't just want to use a computer, but desire to **understand** it to its core.

---

## 🚀 Vision

Munux was born from an evolutionary proposal: to provide an environment that invites the user on a journey from basic usage to mastering the **kernel and terminal**.

Unlike standard Linux distributions that hide complexity, Munux exposes the internal workings of the operating system (Memory, Interrupts, Scheduling) through its own custom kernel implementation, serving as the ultimate learning tool for developers and enthusiasts.

## 🎯 Objectives

- **Build from Scratch**: A lightweight, modular kernel inspired by Unix/Linux concepts.
- **Layered Learning**: Users choose their depth—from high-level usage to low-level kernel hacking.
- **Gamified Mastery**: A progressive system that unlocks "dangerous" commands as the user proves their knowledge.
- **Transparency**: 100% open source code designed to be read and studied, not just executed.

## 💻 Who is Munux for?

- **Students**: Who want to understand how Operating Systems work under the hood.
- **Developers**: Who want to bridge the gap between software and hardware.
- **Enthusiasts**: Who dream of building or understanding their own kernel.
- **Hackers**: Who seek a deep understanding of memory management and system security.

## 🧠 Philosophy

> "I don't want you to just click.  
> I want you to understand what happens **beneath the click**."

Munux is more than software. It is a technical and philosophical learning environment about how machines truly think.

---

## ⚙️ The "Learning OS" Concept (Planned Features)

Munux aims to implement a unique "Gamified Permission System" in its user space:

### Progressive Learning Levels

The system implements five progressive levels that unlock permissions and advanced commands:

| Level | Role | Permissions & Restrictions | Safety |
|:---:|:---:|---|---|
| **1** | **Beginner** | Restricted access, basic commands only | Constant warnings & Guidance |
| **2** | **Apprentice** | More commands unlocked | Warnings before critical actions |
| **3** | **User** | Access to intermediate technical tools | Standard safety checks |
| **4** | **Power User** | Minimal restrictions | No warnings |
| **5** | **Kernel Hacker** | Full control, direct hardware access | **God Mode** (You can break everything) |

### Interactive Terminal & Testing

- **Integrated Tutorials**: A VS Code-style sidebar in the terminal explaining commands in real-time.
- **Skill Checks**: Practical challenges directly in the terminal to "level up."
- **Anti-Cheat**: Logic to ensure the user is actually learning, not just copy-pasting.

---

## 🗺️ Kernel Architecture at a Glance

The diagram below (Mermaid / UML) summarizes how Munux subsystems are wired together. GitHub renders it natively — see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full component and boot-sequence diagrams.

```mermaid
flowchart TB
    HW[("x86 Hardware<br/>CPU · PIC · PIT · KBD · Mouse · IDE · COM · RAM")]
    BOOT[Bootloader + Multiboot]
    IDT[Interrupts<br/>IDT / ISR / IRQ]
    MEM[Memory<br/>PMM → VMM → Heap]
    DRV[Drivers<br/>timer · kbd · mouse · serial · disk]
    PROC[Processes<br/>PCB + Round-Robin Scheduler]
    MAIN([kernel_main])

    HW --> BOOT --> MAIN
    MAIN --> IDT
    MAIN --> MEM
    MAIN --> DRV
    MAIN --> PROC
    DRV --> IDT
    PROC --> MEM
    DRV --> HW
```

## 📌 Technical Status: Version 0.2

The project has reached a significant milestone. The **Munux Kernel** core is functional with all fundamental subsystems implemented in C and Assembly.

### ✅ Implemented (Level 1 Complete)

**Interrupt System**
- Complete IDT with 256 entries
- Handlers for all 32 CPU exceptions
- Hardware IRQ management (remapped PIC)
- Assembly stubs for context saving

**Memory Management**
- **PMM**: Physical Memory Manager with bitmap allocation
- **VMM**: Virtual Memory Manager with paging (Two-level page tables)
- **Heap**: Kernel heap allocator (malloc/free)
- Memory protection and isolation

**Process Management**
- Complete Process Control Block (PCB) structure
- Round-robin scheduler with 4 priority levels
- Preemptive multitasking
- Optimized Context Switching in Assembly

**Device Drivers**
- **Timer**: PIT (Programmable Interval Timer) for scheduling
- **Input**: PS/2 Keyboard (ABNT2 layout) & PS/2 Mouse (3 buttons)
- **Debug**: Serial Port driver for logging
- **Storage**: ATA/IDE Disk Controller (PIO Mode)

### 📋 Roadmap

**Phase 2 - System Services** (Current Focus)
- Virtual File System (VFS) abstraction
- Ext2 file system implementation
- Standard C Library
- System Calls interface
- User Mode (Ring 3) separation

*See [docs/ROADMAP.md](docs/ROADMAP.md) for the detailed development plan.*

---

## 📚 Documentation

We believe documentation is as important as code. Explore the Munux architecture:

- **[BUILD.md](docs/BUILD.md)**: How to compile and run Munux
- **[ARCHITECTURE.md](docs/ARCHITECTURE.md)**: System design overview
- **[MEMORY.md](docs/MEMORY.md)**: How PMM, VMM and Heap work
- **[PROCESSES.md](docs/PROCESSES.md)**: Scheduling and Multitasking internals
- **[INTERRUPTS.md](docs/INTERRUPTS.md)**: IDT and Exception handling
- **[DRIVERS.md](docs/DRIVERS.md)**: Timer, keyboard, mouse, serial and ATA drivers
- **[INDEX.md](docs/INDEX.md)**: Full documentation index (with UML diagrams)

---

## ⚖️ License

This project is licensed under the [GNU General Public License v3.0 (GPLv3)](LICENSE).

---

## ✨ Authorship

Created and maintained by **Munique Alves Pacheco Feitoza**.

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=flat&logo=linkedin)](https://www.linkedin.com/in/munique-feitoza-77034b231/)
[![GitHub](https://img.shields.io/badge/GitHub-Follow-black?style=flat&logo=github)](https://github.com/Munique-Feitoza/)