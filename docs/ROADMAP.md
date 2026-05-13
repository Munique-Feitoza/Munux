# Munux Development Roadmap

## Current Status: v0.3 — Rust Adoption Complete

| Version | Codename | State |
|:---:|---|---|
| **0.2** | *Kernel Fundamentals* | ✅ Released |
| **0.3** | *Rust Adoption* | ✅ Released |
| **0.4** | *System Services* | 🎯 Active development |
| **0.5+** | *User Mode and beyond* | 📋 Long-term |

The v0.3 milestone closes a multi-cycle effort to bring Rust into the Munux source tree without disturbing the existing C ABI. The polyglot build pipeline now produces a single kernel ELF combining objects from three toolchains — NASM, GCC and `rustc` — and the first subsystem (the kernel heap) has been ported to safe Rust under the existing entry points.

The codebase remains predominantly C and Assembly. Rust is **additive**: it joins the toolchain wherever memory safety provides the highest leverage, and old code is left alone until there is a concrete reason to rework it.

## Phase 1: Foundation (COMPLETED)

### Bootloader
- x86 real mode bootloader
- Protected mode transition
- Kernel loading from disk
- GDT configuration

### Basic Kernel
- VGA text mode output
- Terminal with colors and scrolling
- Boot sequence initialization

### Interrupt System
- IDT configuration with 256 entries
- CPU exception handlers (0-31)
- Hardware IRQ handling (32-47)
- PIC reprogramming and EOI handling

### Memory Management
- Physical memory manager with bitmap allocation
- Virtual memory with paging
- Two-level page tables
- Kernel heap allocator with malloc/free
- Memory protection and isolation

### Process Management
- Process Control Block structure
- Round-robin scheduler with priorities
- Context switching in assembly
- Preemptive multitasking

### Device Drivers
- Programmable Interval Timer
- PS/2 keyboard with scancode translation
- PS/2 mouse with three-button support
- Serial port for debugging
- ATA/IDE disk controller

## Phase 1.5 — v0.3 Rust Adoption (COMPLETED)

This phase introduced Rust to the kernel as a `no_std` static library linked into the existing ELF. The user-visible footprint is intentionally small; the foundational impact is significant — every future subsystem now has the option of being written in Rust without further toolchain work. See [RUST.md](RUST.md) for the full strategy.

### Toolchain and Build
- Custom target specification `i686-unknown-none.json` matching the C ABI (no FPU, static relocation, kernel code model)
- `rust-toolchain.toml` pinning a specific nightly channel for reproducible builds
- `Makefile` integration: `make rust`, `make rust-check`, `make rust-fmt`, `make rust-headers`, `make rust-clean`, with the Rust artifact participating in the final link
- `cbindgen` configured to emit C headers consumed by the existing kernel sources; the header is committed to the repository for auditability
- `clippy -D warnings` and `rustfmt --check` enforced as build gates

### FFI Boundary
- `extern "C"` declarations on every exported Rust function, prefixed `munux_rs_`
- A shim crate (`munux-rs-ffi`) hosts all `unsafe` code at the boundary; the rest of the Rust tree compiles under `#![forbid(unsafe_op_in_unsafe_fn)]`
- Panic handler routes through the existing `kernel_panic` path, so a Rust panic surfaces identically to a C panic
- IRQ-safe locking via a handrolled `IrqMutex<T>` that pairs a spin-lock with `cli` / `sti`, reusable by any future Rust subsystem

### First Ported Subsystem — Heap Allocator
- The first-fit free-list allocator from the legacy [`heap.c`](../munux-core/kernel/memory/heap.c) was reimplemented in Rust ([`heap.rs`](../munux-core/kernel/rust/munux-rs/src/heap.rs))
- The C entry points `kmalloc` / `kmalloc_aligned` / `kmalloc_physical` / `kfree` are preserved byte-for-byte and now forward to the Rust implementation
- Block header layout (`#[repr(C)]`) matches the legacy `heap_block_t` for debugger continuity
- Heap growth still happens on the C side (it requires PMM / VMM) and is exposed to Rust via a small `munux_c_heap_grow` callback

### Closed With
- The kernel boots in QEMU with the Rust-backed heap as the sole allocator
- `kernel_main` now runs the full init chain `idt_init → pmm_init → vmm_init → heap_init`, followed by a heap smoke test (`kmalloc(128)`, `kmalloc(256)`, write/read-back, `kfree`, `kmalloc(384)` to exercise coalescing) that completes in green
- All existing C subsystems link and operate unchanged
- The Rust workspace compiles with zero warnings under `-D warnings` and passes `clippy --lib` with documented `#[allow]` exceptions
- Two latent issues uncovered and fixed during wiring:
  1. **`vmm_init` bootstrap**: previously allocated the page directory through the (uninitialized) heap; now takes a physical frame directly from PMM
  2. **C compilation flags**: added `-mno-sse -mno-sse2 -mno-mmx -mgeneral-regs-only` to keep GCC from auto-vectorizing `memset`/`memcpy` into SSE2 instructions the kernel cannot execute

## Phase 2 — v0.4 System Services (ACTIVE)

### File System Layer
- Virtual File System abstraction
- Path resolution and directory traversal
- File descriptor management
- Buffer cache for disk I/O

### ext2 File System
- Superblock and group descriptors
- Inode allocation and management
- Directory entry handling
- File read/write operations
- Hard links and symbolic links

### Standard C Library
- String manipulation (strcpy, strcat, strcmp, etc.)
- Character classification (isalpha, isdigit, etc.)
- Number conversion (atoi, itoa, sprintf)
- Memory functions beyond basic memcpy/memset
- Printf family with format specifiers

### System Calls
- Software interrupt interface (int 0x80)
- Parameter passing via registers
- Return value handling
- Error code conventions
- Basic syscalls: read, write, open, close, exit, fork, exec

## Phase 3: User Mode (FUTURE)

### Privilege Separation
- Ring 3 execution for user processes
- Ring 0 kernel protection
- Syscall entry/exit handling
- Stack switching between modes

### User Space Infrastructure
- ELF executable loading
- Dynamic linking support
- Program segments (text, data, bss)
- Program initialization
- Command line arguments and environment

### Process Isolation
- Separate page directories per process
- Copy-on-write forking
- Process memory limits
- Resource accounting

## Phase 4: Advanced Features (FUTURE)

### Networking
- Network device drivers (e1000, rtl8139)
- ARP protocol implementation
- IPv4 packet handling
- ICMP for ping
- UDP sockets
- TCP state machine
- Socket API (socket, bind, listen, accept, connect, send, recv)

### Graphics
- Framebuffer driver (VESA/UEFI GOP)
- Basic drawing primitives
- Font rendering
- Window management
- Input event routing
- Simple GUI toolkit

### Advanced Memory
- Demand paging with page fault handler
- Page replacement algorithms (LRU, clock)
- Swap space management
- Memory-mapped files
- Shared memory segments

### Advanced Scheduling
- Priority inheritance for locks
- Real-time scheduling class
- CPU affinity and binding
- Load balancing
- Process groups and sessions

## Phase 5: Munux-Specific Features (FUTURE)

### Learning System
- Permission levels (1-5) for progressive access
- Command whitelisting/blacklisting per level
- Contextual warnings for dangerous operations
- Progress tracking and analytics

### Interactive Tutorials
- Built-in tutorial mode
- Step-by-step command guidance
- Explanation panel (VS Code-style sidebar)
- Practice exercises with validation
- Achievement system for milestones

### Testing Framework
- In-terminal testing system
- Multiple choice and practical questions
- Automatic answer validation
- Anti-cheat mechanisms
- Level-up tests between tiers

### Documentation System
- Offline help database
- Man page viewer
- Context-sensitive suggestions
- Search functionality
- Beginner-friendly explanations

### Shell Features
- Custom shell with learning features
- Command history and completion
- Syntax highlighting for learning
- Mistake detection and suggestions
- Progress statistics

## Phase 6: Distribution Building (FUTURE)

### Package Management
- Package format definition
- Dependency resolution
- Installation/removal tools
- Repository structure
- Update mechanisms

### Installer
- Disk partitioning
- File system creation
- Bootloader installation
- Base system installation
- Configuration wizard

### Desktop Environment
- Window manager
- Panel/taskbar
- Application launcher
- File manager
- Settings manager
- Terminal emulator

## Technical Debt and Improvements

### Code Quality
- Comprehensive error handling
- Input validation throughout
- Memory leak detection
- Dead code elimination
- Consistent code style

### Testing
- Unit tests for core functions
- Integration tests for subsystems
- Regression test suite
- Automated testing in CI
- Performance benchmarks

### Documentation
- Inline code documentation
- Architecture diagrams
- API reference completion
- Tutorial content
- Troubleshooting guides

### Performance
- Profiling infrastructure
- Hotspot identification
- Algorithm optimization
- Cache-friendly data structures
- Lock contention reduction

## Long-Term Vision

### Multi-Core Support
- SMP initialization
- Per-CPU data structures
- CPU-local schedulers
- Spinlock implementation
- Inter-processor interrupts

### 64-bit Port
- x86-64 architecture support
- Long mode transition
- 64-bit memory management
- Large memory support
- Modern instruction sets

### Platform Expansion
- ARM architecture port
- RISC-V support
- UEFI boot support
- Device tree parsing
- Platform abstraction layer

### Security Hardening
- ASLR (Address Space Layout Randomization)
- Stack canaries
- NX (No Execute) bit enforcement
- Capability-based security
- Sandboxing mechanisms

## Community and Ecosystem

### Developer Tools
- Kernel debugger improvements
- Profiling tools
- Memory analysis
- Trace logging
- Crash dump analysis

### Application Support
- POSIX API compatibility
- Existing software porting
- Development toolchain
- Libraries and frameworks
- Application ecosystem

### Community Building
- Contributor guidelines
- Code review process
- Communication channels
- Regular releases
- Community governance

---

This roadmap represents the long-term vision for Munux. Implementation priorities may shift based on learning objectives, technical challenges, and community input. The focus remains on creating an educational yet fully functional operating system that demystifies how computers work at the lowest levels.

**Current Focus**: **Phase 2 — System Services (v0.4)**, starting with the VFS abstraction and the ext2 file system. With the polyglot build pipeline established and the FFI contract validated by the heap port, new subsystems default to Rust where the boundary cost is acceptable.

**Starting Points for v0.4**

These are concrete pieces of work that are ready to be picked up immediately. The v0.3 wrap-up surfaced each of them; none are blockers for opening v0.4 but tackling them early avoids them coming back as surprises:

1. **Multiboot memory discovery** — `pmm_init(0x02000000)` currently hard-codes 32 MiB. Parse the multiboot info structure (pointer in `EBX` at entry) to read `mem_upper` and pass the real value.
2. **Aligned-alloc API** — `kmalloc_aligned` currently over-allocates by a page and rounds up, leaking the slack. A first-class aligned-alloc on the Rust side (`munux_rs_alloc_aligned(size, align)`) would clean this up before the VFS / disk-cache starts using it heavily.
3. **VFS skeleton (Rust)** — Single trait for `Filesystem` and `Inode`, plus an in-memory `tmpfs` as the first registration, mountable at `/` for early boot tests.
4. **ext2 superblock and inode parsing (Rust)** — Pure parsing of on-disk metadata, no kernel side effects; perfect for property-based tests on the host.
5. **Buffer cache** — Owned by Rust, fed by the existing C ATA driver via an `extern "C"` `disk_read_sector` / `disk_write_sector` shim.

**Next Milestone**: items 1 and 2 closed; items 3 and 4 producing a `tmpfs` and a read-only ext2 mount visible from `kernel_main`.
