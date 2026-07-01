# Compilando e Executando o Munux

## Pré-requisitos

### Ferramentas Necessárias

Para compilar o Munux, você precisa de uma toolchain de desenvolvimento cruzado que cubra três linguagens.

**Compilador Cruzado de C** — `i686-elf-gcc` e `i686-elf-binutils`
- GCC configurado para o alvo bare-metal i686
- Não faz link com as bibliotecas do host, evitando dependências acidentais do host

**Assembler** — NASM (Netwide Assembler)
- Suporta assembly x86 tanto de 16 bits quanto de 32 bits
- Sintaxe limpa para o bootloader e os stubs do kernel

**Toolchain de Rust** — `rustup` com o canal nightly
- O `rustc` nightly é necessário porque a build depende de `-Z build-std` e de uma especificação de alvo customizada
- O componente `rust-src` é necessário para que as crates `core` e `alloc` do Rust possam ser recompiladas para o alvo
- O `cbindgen` gera os headers C a partir da crate Rust para que o núcleo em C consiga chamá-la

**Utilitários do GRUB** — `grub-mkrescue`
- Cria imagens ISO inicializáveis, tratando o protocolo multiboot

**Emulador** — emulador de sistema QEMU
- Emulação x86 rápida para testes, com suporte embutido a stub do GDB

### Instalando as Dependências

No Ubuntu/Debian:
```bash
sudo apt-get install build-essential nasm qemu-system-x86 grub-pc-bin xorriso
```

No Arch / Manjaro:
```bash
sudo pacman -S base-devel nasm qemu-system-i386 grub libisoburn
```

O compilador cruzado de C precisa ser compilado manualmente ou instalado a partir de um repositório.

#### Instalando a Toolchain de Rust

O instalador recomendado é o `rustup`:

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
rustup toolchain install nightly
rustup component add rust-src --toolchain nightly
cargo install cbindgen
```

O repositório fixa a versão exata do nightly por meio de um arquivo `rust-toolchain.toml` em `munux-os/kernel/rust/`, então o `rustup` baixará automaticamente a versão correta na primeira build.

### Compilando o Compilador Cruzado de C

Se não houver um compilador cruzado pré-compilado disponível:

1. Baixe os fontes do `binutils` e do `gcc`
2. Configure com `--target=i686-elf --disable-nls --without-headers`
3. Compile e instale o `binutils`
4. Compile e instale o `gcc` (apenas o núcleo, sem `libgcc`)

Esse processo pode levar uma hora ou mais em sistemas lentos.

## Sistema de Build

### Estrutura do Makefile

O Makefile orquestra o processo de build:

**Variáveis**: Definem ferramentas, flags e caminhos
**Regras de Padrão**: Compilam .c para .o e .asm para .o
**Dependências**: Garantem a ordem correta de build
**Alvos**: all, clean, run, debug, test

### Processo de Build

A build prossegue em fases:

**Fase 1 — Bootloader**: Monta o `bootloader.asm` em um binário flat

**Fase 2 — Objetos do Kernel (C / Assembly)**: Compila os fontes C com o compilador cruzado freestanding e monta os fontes assembly com o NASM

**Fase 3 — Biblioteca Estática de Rust**: Invoca `cargo build --release --target i686-unknown-none.json -Z build-std=core,alloc` em `kernel/rust/`, produzindo `libmunux_rs.a`

**Fase 4 — Linkagem**: Faz o link de todos os arquivos objeto de C/ASM junto com `libmunux_rs.a` no executável ELF do kernel

**Fase 5 — Extração do Binário**: Extrai um binário flat do ELF para o caminho de floppy/imagem raw

**Fase 6 — Criação da ISO**: Combina o bootloader e o kernel em uma ISO inicializável via `grub-mkrescue`

A fase do Rust roda em paralelo com a fase de C/Assembly quando `make -j` é usado, já que as duas saídas são independentes até o passo final de link.

### Flags de Compilação

**A compilação de C** usa flags específicas de freestanding:

| Flag | Propósito |
|---|---|
| `-ffreestanding` | Sem ambiente hospedado, sem biblioteca padrão |
| `-nostdlib` | Não fazer link com a biblioteca padrão |
| `-m32` | Gerar código de 32 bits |
| `-O2` | Otimização moderada, bom equilíbrio entre velocidade e depurabilidade |
| `-Wall -Wextra` | Habilitar avisos abrangentes |
| `-fno-builtin` | Desabilitar o reconhecimento de funções built-in |
| `-fno-stack-protector` | Sem stack canaries (não suportado em freestanding) |
| `-fno-pic -fno-pie` | Gerar código dependente de posição (o kernel roda em um endereço virtual fixo) |

**A compilação de Rust** usa uma configuração igualmente restrita, declarada em `kernel/rust/.cargo/config.toml` e `i686-unknown-none.json`:

| Configuração | Propósito |
|---|---|
| `#![no_std]` (atributo de crate) | Sem biblioteca padrão do Rust; apenas `core` e `alloc` |
| Alvo customizado `i686-unknown-none` | i686 bare-metal com a ABI System V correspondente ao lado C |
| `panic = "abort"` (em `Cargo.toml`) | Sem stack unwinding — panics são roteados para o handler de panic do kernel |
| `-Z build-std=core,alloc` | Recompilar `core` e `alloc` para o alvo em vez de usar uma cópia do host pré-compilada |
| `RUSTFLAGS="-C relocation-model=static"` | Código dependente de posição correspondente ao `-fno-pic` do lado C |
| `opt-level = "s"` (profile de release) | Otimizar para tamanho; binários de kernel devem se manter compactos |

Essas escolhas garantem que a saída do Rust seja compatível em link com os objetos C e livre de qualquer dependência de runtime oculta.

### Script do Linker

O script do linker controla o layout de memória:

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

Isso posiciona o kernel no endereço físico 0x80000 (512KB) e define as seções padrão.

## Compilando

### Build Rápida

Para compilar o sistema inteiro:

```bash
cd munux-os
make
```

Isso produz `build/munux.iso`, uma imagem de CD inicializável.

### Builds Incrementais

O Make rastreia dependências automaticamente. Alterar um arquivo .c recompila apenas aquele arquivo e refaz o link. Alterar um header recompila todos os arquivos que o incluem.

### Build Limpa

Para remover todos os arquivos gerados:

```bash
make clean
```

Isso garante que a próxima build comece do zero, útil ao investigar problemas de build.

### Verificando Dependências

Para verificar se as ferramentas necessárias estão instaladas:

```bash
make check-deps
```

Isso verifica a presença de `gcc`, `nasm`, `cargo`, `rustup`, `cbindgen`, `grub-mkrescue` e `qemu-system-i386`, reportando quaisquer ferramentas ausentes.

### Compilando Apenas a Camada Rust

Para iterar no código Rust sem recompilar o kernel inteiro:

```bash
make rust       # recompila apenas libmunux_rs.a
make rust-check # cargo check + clippy, sem codegen
make rust-test  # roda os testes unitários no host na crate `tests/`
```

A crate Rust também traz um pequeno conjunto de testes unitários no alvo do host para módulos de lógica pura (estruturas de dados, parsers). Eles rodam no host do desenvolvedor e nunca entram na build do kernel.

## Executando

### Emulação com QEMU

Para iniciar o Munux no QEMU:

```bash
make run
```

Isso inicia o QEMU com:
- 32MB de RAM
- Um único núcleo de CPU
- Display em modo texto VGA
- ISO carregada como CD-ROM

O QEMU disponibiliza uma janela mostrando o console do sistema.

### Modo de Depuração

Para depuração com o GDB:

```bash
make debug
```

Isso inicia o QEMU com:
- Stub do GDB escutando na porta 1234
- Pausado na inicialização aguardando o depurador

Em outro terminal:
```bash
gdb build/kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

### Hardware Real

Para inicializar em hardware real:

1. Grave a munux.iso em um pendrive USB ou queime em um CD
2. Configure a BIOS para inicializar a partir do dispositivo
3. Ligue e observe o boot

**Aviso**: Hardware real pode expor bugs que não aparecem na emulação. Teste exaustivamente no QEMU primeiro.

## Testando

### Testes Automatizados

Para rodar a validação básica:

```bash
make test
```

Isso verifica:
- A existência do arquivo ISO e se ele tem um tamanho razoável
- Se a build não produz avisos
- Um smoke test básico no QEMU

### Testes Manuais

Inicialize o sistema e verifique:
- Se as mensagens de boot aparecem
- Se não há mensagens de erro ou exceções
- Se a entrada do teclado funciona
- Se o cursor do mouse se move
- Se o timer emite ticks na taxa esperada

### Técnicas de Depuração

Quando surgem problemas:

**Log Serial**: Adicione mensagens de depuração via porta serial

**Monitor do QEMU**: Pressione Ctrl+Alt+2 para acessar o monitor do QEMU para introspecção

**GDB**: Use depuração em nível de código-fonte para percorrer o código

**bochs**: Emulador alternativo com depurador embutido

**Hardware Real**: Às vezes revela problemas de timing ou específicos de hardware

## Resolução de Problemas

### Problemas Comuns

**Compilador Cruzado Não Encontrado**: Garanta que o `i686-elf-gcc` esteja no `PATH`

**Permissão Negada no make**: O diretório de build pode precisar ser criado manualmente

**QEMU Trava**: Verifique se VT-x/AMD-V está habilitado na BIOS

**Boot Loop**: O bootloader pode não estar carregando o kernel corretamente

**Tela Preta**: A inicialização de vídeo pode ter falhado

**Sem Teclado**: A emulação PS/2 precisa estar habilitada no QEMU

**`error[E0463]: can't find crate for 'core'`**: O componente `rust-src` está ausente — rode `rustup component add rust-src --toolchain nightly`

**`linker 'rust-lld' not found`**: Instale o componente `llvm-tools-preview` ou sobrescreva o linker em `.cargo/config.toml` para usar o `ld` do sistema

**Convenção de chamada divergente entre C e Rust**: Confirme que toda função entre linguagens carrega `extern "C"` no lado Rust e corresponde ao protótipo C byte a byte; regenere os headers com `cbindgen` após alterar qualquer assinatura de FFI

### Saída de Depuração

Habilite a depuração verbosa:

```c
serial_writestring(COM1, "Debug: reached this point\n");
```

A saída serial aparece no console do QEMU ou pode ser redirecionada para um arquivo:

```bash
qemu-system-i386 -serial file:debug.log ...
```

### Corrupção de Memória

Se o sistema trava de forma imprevisível:

1. Verifique se há buffer overflows
2. Verifique a validade dos ponteiros
3. Garanta que as stacks sejam grandes o suficiente
4. Verifique se há erros de use-after-free

As opções -d do QEMU podem logar todos os acessos à memória.

## Profiling de Performance

### Medição de Tempo

Use o timer para medir a execução do código:

```c
uint32_t start = timer_get_ticks();
// ... code to measure ...
uint32_t end = timer_get_ticks();
uint32_t elapsed = end - start;  // In timer ticks
```

### Otimização

Faça profiling antes de otimizar. Gargalos comuns:

- Algoritmos ineficientes (use estruturas de dados melhores)
- Latência excessiva de interrupção (encurte os handlers)
- Overhead do alocador de memória (alocação por pool para tamanhos fixos)
- Overhead de troca de contexto (reduza a frequência de troca)

A otimização deve focar em gargalos medidos, não em especulação.

## Integração Contínua

### Builds Automatizadas

Um sistema de CI deve:

1. Fazer o checkout do código-fonte
2. Compilar o sistema completo
3. Rodar os testes automatizados
4. Gerar os artefatos de build
5. Reportar falhas

GitHub Actions, GitLab CI ou Jenkins podem automatizar esse fluxo de trabalho.

### Processo de Release

Para releases:

1. Marque a versão com uma tag no git
2. Faça uma build limpa a partir da tag
3. Teste exaustivamente
4. Gere o changelog
5. Crie as notas de release
6. Publique a ISO e a documentação

Builds reproduzíveis garantem que as ISOs distribuídas correspondam às builds a partir do fonte.

---

**Veja Também**:
- README.md - Visão geral do projeto
- ARCHITECTURE.md - Design do sistema
- CONTRIBUTING.md - Como contribuir (quando criado)
