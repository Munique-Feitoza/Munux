# Munux Rust Subsystem

`no_std`, freestanding Rust workspace built as a static library and linked into the kernel ELF.

See [../../../docs/RUST.md](../../../docs/RUST.md) for the full integration strategy. Top-level layout:

```
rust/
├── Cargo.toml                # Workspace manifest
├── rust-toolchain.toml       # Pinned nightly
├── i686-unknown-none.json    # Custom target spec (no FPU, static reloc, kernel code model)
├── .cargo/config.toml        # Default --target and rustflags
├── cbindgen.toml             # C header generation
├── munux-rs/                 # Safe-Rust core (forbid unsafe outside narrow exceptions)
│   ├── src/lib.rs
│   ├── src/heap.rs           # First-fit allocator with coalescing
│   └── src/sync.rs           # IrqMutex (cli/sti + spinlock, SMP-ready)
├── munux-rs-ffi/             # FFI shim (only place unsafe extern "C" lives)
│   ├── src/lib.rs            # Public C ABI exports
│   └── src/panic.rs          # #[panic_handler] → kernel_panic
└── include/munux_rs.h        # Generated, committed header
```

## Local commands (from `munux-core/`)

```
make rust           # cargo build --release → libmunux_rs.a
make rust-check     # cargo check + clippy -D warnings
make rust-fmt       # cargo fmt --check
make rust-headers   # regenerate include/munux_rs.h via cbindgen
make rust-clean     # cargo clean
```

The Rust phase parallelizes with the C/Assembly compilation under `make -j`.
