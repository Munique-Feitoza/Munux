# Subsistema Rust do Munux

Workspace Rust `no_std` e freestanding, compilado como biblioteca estática e linkado ao ELF do kernel.

Veja [../../docs/RUST.md](../../docs/RUST.md) para a estratégia de integração completa. Layout de mais alto nível:

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

## Comandos locais (a partir de `munux-os/`)

```
make rust           # cargo build --release → libmunux_rs.a
make rust-check     # cargo check + clippy -D warnings
make rust-fmt       # cargo fmt --check
make rust-headers   # regenerate include/munux_rs.h via cbindgen
make rust-clean     # cargo clean
```

A fase Rust roda em paralelo com a compilação C/Assembly sob `make -j`.
