# Building and Running Munux

## Prerequisites

### Required Tools

To build Munux, you need a cross-development toolchain covering three languages.

**C Cross Compiler** — `i686-elf-gcc` and `i686-elf-binutils`
- GCC configured for the bare-metal i686 target
- Does not link against host libraries, preventing accidental host dependencies

**Assembler** — NASM (Netwide Assembler)
- Supports both 16-bit and 32-bit x86 assembly
- Clean syntax for the bootloader and kernel stubs

**Rust Toolchain** — `rustup` with the nightly channel
- `rustc` nightly is required because the build relies on `-Z build-std` and a custom target specification
- The `rust-src` component is required so Rust's `core` and `alloc` crates can be rebuilt for the target
- `cbindgen` generates C headers from the Rust crate so the C core can call into it

**GRUB Utilities** — `grub-mkrescue`
- Creates bootable ISO images, handling the multiboot protocol

**Emulator** — QEMU system emulator
- Fast x86 emulation for testing, with built-in GDB stub support

### Installing Dependencies

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential nasm qemu-system-x86 grub-pc-bin xorriso
```

On Arch / Manjaro:
```bash
sudo pacman -S base-devel nasm qemu-system-i386 grub libisoburn
```

The C cross compiler must be built manually or installed from a repository.

#### Installing the Rust Toolchain

The recommended installer is `rustup`:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup toolchain install nightly
rustup component add rust-src --toolchain nightly
cargo install cbindgen
```

The repository pins the exact nightly version through a `rust-toolchain.toml` file in `munux-core/kernel/rust/`, so `rustup` will automatically download the correct version on first build.

### Building the C Cross Compiler

If a pre-built cross compiler isn't available:

1. Download `binutils` and `gcc` sources
2. Configure with `--target=i686-elf --disable-nls --without-headers`
3. Build and install `binutils`
4. Build and install `gcc` (core only, no `libgcc`)

This process can take an hour or more on slow systems.

## Build System

### Makefile Structure

The Makefile orchestrates the build process:

**Variables**: Define tools, flags, and paths
**Pattern Rules**: Compile .c to .o and .asm to .o
**Dependencies**: Ensure correct build order
**Targets**: all, clean, run, debug, test

### Build Process

Building proceeds in phases:

**Phase 1 — Bootloader**: Assemble `bootloader.asm` to flat binary

**Phase 2 — Kernel Objects (C / Assembly)**: Compile C sources with the freestanding cross compiler and assemble assembly sources with NASM

**Phase 3 — Rust Static Library**: Invoke `cargo build --release --target i686-unknown-none.json -Z build-std=core,alloc` in `kernel/rust/`, producing `libmunux_rs.a`

**Phase 4 — Linking**: Link all C/ASM object files together with `libmunux_rs.a` into the kernel ELF executable

**Phase 5 — Binary Extraction**: Extract a flat binary from the ELF for the floppy/raw-image path

**Phase 6 — ISO Creation**: Combine the bootloader and kernel into a bootable ISO via `grub-mkrescue`

The Rust phase runs in parallel with the C/Assembly phase when `make -j` is used, since the two outputs are independent until the final link step.

### Compilation Flags

**C compilation** uses freestanding-specific flags:

| Flag | Purpose |
|---|---|
| `-ffreestanding` | No hosted environment, no standard library |
| `-nostdlib` | Don't link against the standard library |
| `-m32` | Generate 32-bit code |
| `-O2` | Moderate optimization, good balance of speed and debuggability |
| `-Wall -Wextra` | Enable comprehensive warnings |
| `-fno-builtin` | Disable built-in function recognition |
| `-fno-stack-protector` | No stack canaries (not supported in freestanding) |
| `-fno-pic -fno-pie` | Generate position-dependent code (the kernel runs at a fixed virtual address) |

**Rust compilation** uses an equally constrained configuration, declared in `kernel/rust/.cargo/config.toml` and `i686-unknown-none.json`:

| Setting | Purpose |
|---|---|
| `#![no_std]` (crate attribute) | No Rust standard library; only `core` and `alloc` |
| Custom target `i686-unknown-none` | Bare-metal i686 with the System V ABI matching the C side |
| `panic = "abort"` (in `Cargo.toml`) | No stack unwinding — panics route to the kernel panic handler |
| `-Z build-std=core,alloc` | Rebuild `core` and `alloc` for the target instead of using a precompiled host copy |
| `RUSTFLAGS="-C relocation-model=static"` | Position-dependent code matching `-fno-pic` on the C side |
| `opt-level = "s"` (release profile) | Optimize for size; kernel binaries should stay compact |

These choices ensure the Rust output is link-compatible with the C objects and free of any hidden runtime dependencies.

### Linker Script

The linker script controls memory layout:

```
ENTRY(kernel_main)

SECTIONS {
    . = 0x00080000;
    
    .text : { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) *(COMMON) }
}
```

This places the kernel at physical address 0x80000 (512KB) and defines standard sections.

## Building

### Quick Build

To build the entire system:

```bash
cd munux-core
make
```

This produces `build/munux.iso`, a bootable CD image.

### Incremental Builds

Make automatically tracks dependencies. Changing a .c file recompiles only that file and relinks. Changing a header recompiles all files that include it.

### Clean Build

To remove all generated files:

```bash
make clean
```

This ensures the next build starts fresh, useful when troubleshooting build issues.

### Checking Dependencies

To verify required tools are installed:

```bash
make check-deps
```

This checks for `gcc`, `nasm`, `cargo`, `rustup`, `cbindgen`, `grub-mkrescue`, and `qemu-system-i386`, reporting any missing tools.

### Building Only the Rust Layer

For iterating on Rust code without rebuilding the whole kernel:

```bash
make rust       # rebuilds libmunux_rs.a only
make rust-check # cargo check + clippy, no codegen
make rust-test  # runs the host-side unit tests in the `tests/` crate
```

The Rust crate also ships a small set of host-target unit tests for pure-logic modules (data structures, parsers). These run on the developer's host and never enter the kernel build.

## Running

### QEMU Emulation

To boot Munux in QEMU:

```bash
make run
```

This starts QEMU with:
- 32MB RAM
- Single CPU core
- VGA text mode display
- ISO loaded as CD-ROM

QEMU provides a window showing the system console.

### Debug Mode

For debugging with GDB:

```bash
make debug
```

This starts QEMU with:
- GDB stub listening on port 1234
- Paused at startup waiting for debugger

In another terminal:
```bash
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Real Hardware

To boot on real hardware:

1. Write munux.iso to USB drive or burn to CD
2. Configure BIOS to boot from the device
3. Power on and watch it boot

**Warning**: Real hardware may expose bugs not visible in emulation. Test thoroughly in QEMU first.

## Testing

### Automated Tests

To run basic validation:

```bash
make test
```

This checks:
- ISO file exists and has reasonable size
- Build produces no warnings
- Basic smoke test in QEMU

### Manual Testing

Boot the system and verify:
- Boot messages appear
- No error messages or exceptions
- Keyboard input works
- Mouse cursor moves
- Timer ticks at expected rate

### Debugging Techniques

When problems occur:

**Serial Logging**: Add debug messages via serial port

**QEMU Monitor**: Press Ctrl+Alt+2 to access QEMU monitor for introspection

**GDB**: Use source-level debugging to step through code

**bochs**: Alternative emulator with built-in debugger

**Real Hardware**: Sometimes reveals timing or hardware-specific issues

## Troubleshooting

### Common Issues

**Cross Compiler Not Found**: Ensure `i686-elf-gcc` is in `PATH`

**Permission Denied on make**: The build directory may need manual creation

**QEMU Hangs**: Check if VT-x/AMD-V is enabled in BIOS

**Boot Loop**: The bootloader may not be loading the kernel correctly

**Black Screen**: Video initialization may have failed

**No Keyboard**: PS/2 emulation must be enabled in QEMU

**`error[E0463]: can't find crate for 'core'`**: The `rust-src` component is missing — run `rustup component add rust-src --toolchain nightly`

**`linker 'rust-lld' not found`**: Install the `llvm-tools-preview` component or override the linker in `.cargo/config.toml` to use the system `ld`

**Mismatched calling convention between C and Rust**: Confirm every cross-language function carries `extern "C"` on the Rust side and matches the C prototype byte-for-byte; regenerate headers with `cbindgen` after changing any FFI signature

### Debug Output

Enable verbose debugging:

```c
serial_writestring(COM1, "Debug: reached this point\n");
```

Serial output appears in QEMU console or can be redirected to file:

```bash
qemu-system-i386 -serial file:debug.log ...
```

### Memory Corruption

If the system crashes unpredictably:

1. Check for buffer overflows
2. Verify pointer validity
3. Ensure stacks are large enough
4. Check for use-after-free errors

QEMU's -d options can log all memory accesses.

## Performance Profiling

### Timing

Use the timer to measure code execution:

```c
uint32_t start = timer_get_ticks();
// ... code to measure ...
uint32_t end = timer_get_ticks();
uint32_t elapsed = end - start;  // In timer ticks
```

### Optimization

Profile before optimizing. Common bottlenecks:

- Inefficient algorithms (use better data structures)
- Excessive interrupt latency (shorten handlers)
- Memory allocator overhead (pool allocation for fixed sizes)
- Context switch overhead (reduce switching frequency)

Optimization should focus on measured bottlenecks, not speculation.

## Continuous Integration

### Automated Builds

A CI system should:

1. Check out source code
2. Build complete system
3. Run automated tests
4. Generate build artifacts
5. Report failures

GitHub Actions, GitLab CI, or Jenkins can automate this workflow.

### Release Process

For releases:

1. Tag version in git
2. Build clean from tag
3. Test thoroughly
4. Generate changelog
5. Create release notes
6. Publish ISO and documentation

Reproducible builds ensure distributed ISOs match builds from source.

---

**See Also**:
- README.md - Project overview
- ARCHITECTURE.md - System design
- CONTRIBUTING.md - How to contribute (when created)
