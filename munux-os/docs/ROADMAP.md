# Roadmap — Munux OS

> Flavor de **propósito geral** do ecossistema Munux (kernel x86, C/ASM/Rust).
> Legenda: **`[x]`** feito e validado · **`[ ]`** a fazer.
> Meta de ecossistema: o munux-os é o **painel de controle** que vira **cliente MJP** e
> submete jobs ao [munux-zos](../../munux-zos/) pela ponte — ver a fase 🌉 no fim.

## Panorama

|    Versão    | Codinome                                      | Estado                              |
| :------------: | --------------------------------------------- | ----------------------------------- |
| **0.2** | Kernel Fundamentals                           | ✅ base + integração viva (v0.4a) |
| **0.3** | Rust Adoption                                 | ✅ concluída e validada            |
| **0.4** | System Services                               | ✅ concluída e verificada       |
| **0.5** | User Mode                                     | 📋 planejada                        |
| **0.6+** | Learning OS · Rede · Gráficos · Ponte MJP | 📋 longo prazo                      |

---

## v0.2 — Kernel Fundamentals ✅

### Boot & terminal

- [X] Bootloader real mode → protected mode + GDT
- [X] Multiboot
- [X] Terminal VGA (cores + scroll)

### Interrupções

- [X] IDT com 256 entradas
- [X] Handlers das 32 exceções da CPU
- [X] IRQs de hardware: remap do PIC + EOI
- [X] Stubs em Assembly (salvamento de contexto)

### Memória

- [X] PMM — alocador físico por bitmap (frames de 4 KiB)
- [X] VMM — paginação com page directory + page tables (2 níveis)
- [X] Heap — ver v0.3 (agora em Rust)

### Processos — *código escrito, ainda não integrado ao boot*

- [X] Estrutura de PCB
- [X] Scheduler round-robin com prioridades (código)
- [X] Context switch em Assembly

### Drivers — *timer/teclado/serial integrados no boot (v0.4a); mouse/disco ainda não*

- [X] Timer PIT — **ativo no boot** (IRQ0)
- [X] Teclado PS/2 (layout ABNT2, modificadores, buffer circular) — **ativo no boot** (IRQ1)
- [X] Serial (RS-232) — **ativo no boot** (log/console)
- [X] Mouse PS/2 (3 botões) — *código pronto, ainda não ligado*
- [X] Disco ATA/IDE (PIO por polling) — **ativo**: `disk_init` no boot + leitura via ext2 quando há disco

## v0.3 — Rust Adoption ✅

- [X] Target custom `i686-unknown-none` (sem FPU, static reloc, kernel code model)
- [X] `rust-toolchain.toml` fixando nightly reprodutível
- [X] Makefile integra `cargo build` → `libmunux_rs.a` no link final
- [X] `cbindgen` gera `munux_rs.h` (commitado para auditoria)
- [X] Workspace de 2 crates: `munux-rs` (safe) + `munux-rs-ffi` (todo unsafe na borda)
- [X] `IrqMutex<T>` (spin-lock + `cli`/`sti`) reutilizável
- [X] Panic do Rust roteado pro `kernel_panic`
- [X] **Heap portado pra Rust**: first-fit + coalescência, sob `kmalloc`/`kfree` intactos
- [X] Boota em QEMU com o heap Rust como único alocador + smoke test verde
- [X] `clippy -D warnings` e `rustfmt --check` como gates de build

---

## v0.4 — System Services ✅ (concluída e verificada)

> **Fase concluída e verificada em QEMU** (boot headless). Cada subsistema tem um smoke
> test na serial. O OS deixou de "bootar e travar": liga interrupções, memória, arquivos
> e syscalls, e chega num shell interativo (`munux>`), sem triple-fault. Gates limpos:
> build sem warnings, `clippy -D warnings`, `rustfmt`.

### Integração: interrupções vivas + shell

> Dois **bugs latentes** foram achados e corrigidos aqui — ambos explodiriam no `sti`.

- [X] `serial_init` / `timer_init` / `keyboard_init` chamados no `kernel_main`
- [X] **Remap do PIC** (master 0x20 / slave 0x28) — *estava faltando*; sem ele a IRQ0
  caía no vetor 8 (Double Fault)
- [X] **Correção dos stubs Assembly**: passar `registers_t*` (`push esp`) antes do `call`
  — *bug latente*; sem isso o handler C recebia lixo como `regs`
- [X] `irq_handler` despacha para os handlers registrados + EOI correto (master/slave)
- [X] Habilitar interrupções (`sti`) após montar a IDT
- [X] Rotear IRQ0 (timer) → contador de ticks · IRQ1 (teclado) → buffer de entrada
- [X] Auto-teste de boot que **prova** IRQ0 disparando (ticks avançam via interrupção)
- [X] Shell interativo (`help`, `clear`, `about`, `ticks`, `echo`) lendo do teclado

### Descoberta de memória

- [X] Ler multiboot info (ponteiro em `EBX`) e usar `mem_upper` real
  *(verificado em QEMU: `-m 32M`→32 MiB, `-m 128M`→127 MiB; fallback 32 MiB sem multiboot)*

### Aligned alloc

- [X] `munux_rs_alloc_aligned(size, align)` de 1ª classe no Rust, com `kfree_aligned` dedicado
  *(back-pointer para o bloco real; smoke test de boot confirma alinhamento à página)*

### VFS (Rust)

- [x] `#[global_allocator]` ligado ao heap → coleções do `alloc` (`Vec`/`BTreeMap`/`String`) no kernel
- [x] Trait `Filesystem` (create/write/read/list/exists) + `NodeKind`
- [x] `tmpfs` em memória montado em `/` *(smoke test de boot: cria `/etc/motd`, escreve, lê de volta, lista; clippy + rustfmt limpos)*
- [x] Abstração de **inode** (`stat`: tipo + tamanho) + tabela de descritores (fd)

### ext2 (Rust) — leitura

- [x] Parsing de superblock + group descriptors
- [x] Leitura de inode (ponteiros diretos)
- [x] Entradas de diretório (listagem)
- [x] Leitura de arquivo com blocos **diretos e indiretos** (simples + duplo) *(verificado contra imagem real do `mke2fs`: `hello.txt` + `big.bin` de 20 KiB)*

### libk (freestanding)

- [X] `string`: strcmp/strncmp/strcpy/strncpy/strcat/strchr *(já usada pelo shell)*
- [X] `ctype`: isalpha/isdigit/isspace/isupper/islower/toupper/tolower/isalnum
- [X] Conversão: atoi/itoa/utoa *(aritmética 32-bit, sem helpers de 64-bit)*
- [x] `ksnprintf`/`kvsnprintf` (bounds-checked): `%d %u %x %X %s %c %p %%` + flag `0`/width *(smoke test de boot)*

### Syscalls

- [x] Interface por `int 0x80` (gate DPL=3 pronto p/ user mode, stub dedicado)
- [x] Passagem de parâmetros por registradores + retorno em `eax` *(smoke test via int 0x80)*
- [x] `write` (fd=1 → tela/serial) e `get_ticks`
- [x] `open`/`read`/`close` sobre o VFS (tabela de descritores) *(smoke test lê `/etc/motd` via int 0x80)*

### Buffer cache

- [x] Cache de setores (Rust) sobre `disk_read_sector`/`disk_write_sector`, com hit/miss *(usado pelo ext2)*

## v0.5 — User Mode 📋

- [ ] Separação Ring 3 / Ring 0
- [ ] Entrada/saída de syscall + troca de stack entre modos
- [ ] Loader de ELF (text/data/bss, args, env)
- [ ] Page directory separado por processo
- [ ] `fork` copy-on-write
- [ ] Syscalls de processo: `exit` / `fork` / `exec` (precisam de user mode)
- [ ] Limites de memória + accounting de recursos

---

## 🌉 v0.6 — Ponte MJP: munux-os como cliente

> A vitrine do ecossistema: **conectar, não fundir**. O munux-os passa a **submeter jobs**
> ao back-end transacional [munux-zos](../../munux-zos/) e mostrar os resultados.
> Depende de transporte (rede/serial) — casa com a fase de rede. Contrato:
> [munux-zos/docs/PROTOCOL.md](../../munux-zos/docs/PROTOCOL.md).

- [ ] Cliente MJP: montar e enviar frames `SUBMIT` (JCL) com validação Zero Trust
- [ ] Receber `ACK` / `STATUS` / `SYSOUT` / `RESULT` e exibir no terminal
- [ ] Comando de shell `submit <arquivo.jcl>`
- [ ] Painel de acompanhamento de jobs (o "console de operador" do lado cliente)
- [ ] Tradução ASCII↔EBCDIC coerente com o servidor

## Longo prazo 📋

### Sistema de arquivos

- [ ] ext2 **escrita**: alocação de blocos/inodes (bitmaps) + inserção de dir entries
- [ ] Hard links e symlinks · blocos triplamente indiretos

### Rede

- [ ] Drivers (e1000 / rtl8139)
- [ ] ARP · IPv4 · ICMP (ping)
- [ ] UDP · máquina de estados TCP
- [ ] API de sockets (socket/bind/listen/accept/connect/send/recv)

### Gráficos

- [ ] Framebuffer (VESA/GOP) + primitivas de desenho
- [ ] Fontes, janelas, roteamento de eventos, toolkit de GUI simples

### Learning OS (identidade do munux-os)

- [ ] Níveis de permissão gamificados (1–5)
- [ ] Tutoriais interativos + painel explicativo estilo VS Code
- [ ] Framework de testes no terminal + anti-cheat + level-up

### Plataforma & hardening

- [ ] SMP (multi-core) · IPIs · per-CPU
- [ ] Porte 64-bit (long mode)
- [ ] ASLR · stack canaries · enforcement do bit NX

