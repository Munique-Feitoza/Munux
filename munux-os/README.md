# Munux OS

> **Flavor 1 do ecossistema Munux** — o kernel de propósito geral / workspace reativo (x86).
> Para a visão do ecossistema e o outro flavor (`munux-zos`), veja o [README raiz](../README.md).

Este é o kernel de propósito geral do ecossistema Munux — um kernel de SO poliglota que implementa conceitos fundamentais, incluindo gerenciamento de memória, escalonamento de processos, tratamento de interrupções e drivers de dispositivo. O código-fonte é dividido entre **C (C99, freestanding)**, **Assembly x86 (NASM)** e **Rust (`no_std`, edição 2021)**, com cada linguagem usada onde oferece mais vantagem.

## Funcionalidades

### Gerenciamento de Interrupções
- IDT (Interrupt Descriptor Table) completa com 256 entradas
- Handlers de exceção da CPU para todas as exceções x86
- Gerenciamento de IRQ de hardware com reprogramação do PIC
- Sistema extensível de registro de handlers de interrupção

### Gerenciamento de Memória
- **Physical Memory Manager**: Alocação de frames baseada em bitmap para páginas de 4KB
- **Virtual Memory Manager**: Suporte completo a paging com page directory e page tables
- **Heap Allocator**: Alocador first-fit com coalescência de blocos adjacentes, implementado em Rust seguro; exposto pelos pontos de entrada C `kmalloc` / `kfree` já existentes
- Proteção de memória e isolamento entre espaços do kernel
- Locking seguro para IRQ via um `IrqMutex<T>` feito à mão, pronto para SMP

### Gerenciamento de Processos
- Estrutura de Process Control Block (PCB) com rastreamento completo de estado
- Scheduler round-robin com quatro níveis de prioridade
- Multitarefa preemptiva com fatiamento de tempo baseado em quantum
- Troca de contexto implementada em assembly otimizado
- Suporte a criação, terminação e transições de estado de processos

### Drivers de Dispositivo
- **Timer (PIT)**: Programmable Interval Timer para escalonamento e marcação de tempo
- **Teclado**: Driver de teclado PS/2 completo com layout ABNT2, teclas modificadoras e buffer circular
- **Mouse**: Driver de mouse PS/2 com suporte a três botões e rastreamento de movimento
- **Porta Serial**: Comunicação RS-232 para depuração e comunicação com dispositivos externos
- **Disco**: Controladora de disco ATA/IDE com operações de leitura/escrita a nível de setor

### Subsistema Rust (v0.3+)

Um workspace Rust `no_std` em [`kernel/rust/`](kernel/rust/) compila para uma biblioteca estática (`libmunux_rs.a`) que é linkada no ELF do kernel. Ele hospeda o heap allocator e as primitivas compartilhadas de Rust seguro (`IrqMutex`). A superfície de FFI é gerada pelo `cbindgen` e commitada em [`kernel/rust/include/munux_rs.h`](kernel/rust/include/munux_rs.h).

## Compilando

### Pré-requisitos

- **i686-elf-gcc**: Cross-compiler para x86 bare-metal
- **i686-elf-binutils**: Utilitários binários (linker, assembler)
- **NASM**: Netwide Assembler para o bootloader e stubs
- **rustup** + **toolchain nightly** + componente `rust-src`: workspace Rust em [`kernel/rust/`](kernel/rust/)
- **cbindgen**: Gerador de headers C (necessário apenas ao regenerar `munux_rs.h`)
- **GRUB**: `grub-mkrescue` para criar ISOs bootáveis
- **QEMU**: Emulador de sistema x86 para testes

### Compilação

```bash
make             # Compila tudo (C + Assembly + Rust)
make clean       # Remove os artefatos de build
make run         # Compila e executa no QEMU
make debug       # Compila e inicia com o servidor GDB
make test        # Executa os testes automatizados
make rust        # Compila apenas a libmunux_rs.a
make rust-check  # cargo check + clippy -D warnings
make rust-fmt    # cargo fmt --check
make rust-headers  # regenera kernel/rust/include/munux_rs.h
```

## Documentação

A documentação abrangente fica em [`docs/`](docs/) (específica deste flavor):

- [**INDEX.md**](docs/INDEX.md): Guia de navegação da documentação
- [**ARCHITECTURE.md**](docs/ARCHITECTURE.md): Design do sistema e visão geral dos componentes
- [**RUST.md**](docs/RUST.md): Estratégia de integração do Rust, fronteira de FFI e plano de portabilidade
- [**MEMORY.md**](docs/MEMORY.md): Detalhes do subsistema de gerenciamento de memória
- [**PROCESSES.md**](docs/PROCESSES.md): Gerenciamento e escalonamento de processos
- [**INTERRUPTS.md**](docs/INTERRUPTS.md): Sistema de tratamento de interrupções
- [**DRIVERS.md**](docs/DRIVERS.md): Arquitetura dos drivers de dispositivo
- [**BUILD.md**](docs/BUILD.md): Procedimentos de build e teste
- [**API.md**](docs/API.md): Referência completa da API do kernel
- [**ROADMAP.md**](docs/ROADMAP.md): Roadmap de desenvolvimento e planos futuros

Para a visão de todo o ecossistema (ambos os flavors + a ponte MJP), veja [`../docs/ECOSYSTEM.md`](../docs/ECOSYSTEM.md).

## Licença

Este projeto é licenciado sob a GPLv3.

## Autora

Munique Feitoza

## Repositório

https://github.com/Munique-Feitoza/Munux
