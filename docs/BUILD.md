# Building and Running Munux

## Prerequisites

### Required Tools

To build Munux, you need:

**Cross Compiler**: i686-elf-gcc and i686-elf-binutils
- GCC configured for bare-metal i686 target
- Does not link against host libraries
- Prevents accidental host dependencies

**Assembler**: NASM (Netwide Assembler)
- Supports both 16-bit and 32-bit x86 assembly
- Clean syntax for bootloader and kernel stubs

**GRUB Utilities**: grub-mkrescue
- Creates bootable ISO images
- Handles multiboot protocol
- Available in most Linux distributions

**Emulator**: QEMU system emulator
- Fast x86 emulation for testing
- Supports debugging features
- Can emulate various hardware configurations

### Installing Dependencies

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential nasm qemu-system-x86 grub-pc-bin xorriso
```

The cross compiler must be built manually or installed from a repository.

### Building Cross Compiler

If a pre-built cross compiler isn't available:

1. Download binutils and gcc sources
2. Configure with --target=i686-elf --disable-nls --without-headers
3. Build and install binutils
4. Build and install gcc (core only, no libgcc)

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

**Phase 1 - Bootloader**: Assemble bootloader.asm to flat binary
**Phase 2 - Kernel Objects**: Compile C sources and assemble assembly sources
**Phase 3 - Linking**: Link all objects into kernel ELF executable  
**Phase 4 - Binary Extraction**: Extract flat binary from ELF for loading
**Phase 5 - ISO Creation**: Combine bootloader and kernel into bootable ISO

### Compilation Flags

C compilation uses specific flags:

**-ffreestanding**: No hosted environment, no standard library
**-nostdlib**: Don't link against standard library
**-m32**: Generate 32-bit code
**-O2**: Moderate optimization, good balance of speed and debuggability
**-Wall -Wextra**: Enable comprehensive warnings
**-fno-builtin**: Disable built-in function recognition
**-fno-stack-protector**: No stack canaries (not supported in freestanding)

These flags ensure the kernel doesn't depend on hosted environment features.

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

This checks for gcc, nasm, grub-mkrescue, and qemu, reporting any missing tools.

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

**Cross Compiler Not Found**: Ensure i686-elf-gcc is in PATH

**Permission Denied on make**: Build directory may need manual creation

**QEMU Hangs**: Check if VT-x/AMD-V is enabled in BIOS

**Boot Loop**: Bootloader may not be loading kernel correctly

**Black Screen**: Video initialization may have failed

**No Keyboard**: PS/2 emulation must be enabled in QEMU

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
