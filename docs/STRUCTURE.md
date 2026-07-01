# Estrutura do Projeto Munux

## Organização do Repositório

O repositório do Munux é organizado para separar responsabilidades e manter a clareza:

```
Munux/
├── AUTHORSHIP.md              # Declaração de autoria e créditos
├── LICENSE                    # Texto da licença GPLv3
├── README.md                  # Visão geral e visão do projeto
├── docs/                      # Documentação do ECOSSISTEMA (projeto inteiro)
│   ├── ECOSYSTEM.md          # Visão geral, flavors e a ponte MJP
│   └── STRUCTURE.md          # Este arquivo
├── munux-os/               # Flavor 1: workspace reativo (kernel x86 — C/ASM/Rust)
    ├── README.md             # Documentação do kernel comum
    ├── Makefile              # Sistema de build (GCC + NASM + Cargo + LD)
    ├── docs/                 # Documentação SÓ do munux-os
    │   ├── INDEX.md          # Guia de navegação da doc do OS
    │   ├── ARCHITECTURE.md   # Visão de arquitetura do kernel
    │   ├── RUST.md           # Integração Rust e fronteira FFI
    │   ├── MEMORY.md         # PMM / VMM / Heap
    │   ├── PROCESSES.md      # Processos e scheduler
    │   ├── INTERRUPTS.md     # IDT e exceções
    │   ├── DRIVERS.md        # Drivers de dispositivo
    │   ├── BUILD.md          # Build e testes
    │   ├── API.md            # Referência de API do kernel
    │   └── ROADMAP.md        # Roadmap do munux-os
    ├── boot/                 # Código do bootloader (Assembly)
    │   ├── bootloader.asm   # Bootloader principal
    │   └── pmode.asm        # Transição para o modo protegido
    ├── kernel/               # Código-fonte do kernel
    │   ├── kernel.c         # Inicialização principal do kernel
    │   ├── kernel.h         # Header e tipos do kernel
    │   ├── kernel.ld        # Linker script
    │   ├── interrupts/      # Subsistema de tratamento de interrupções (C + Assembly)
    │   │   ├── idt.c        # Implementação da IDT
    │   │   ├── idt.h        # Header da IDT
    │   │   ├── interrupt.asm  # Stubs de interrupção em Assembly
    │   │   └── io.h         # Operações de I/O de porta
    │   ├── memory/          # Subsistema de gerenciamento de memória (C)
    │   │   ├── memory.h     # Header do subsistema de memória
    │   │   ├── pmm.c        # Gerenciador de memória física
    │   │   ├── vmm.c        # Gerenciador de memória virtual
    │   │   ├── heap.c       # Bootstrap C: mapeia páginas + grow, encaminha ao Rust
    │   │   └── utils.c      # Utilitários de memória
    │   ├── process/         # Subsistema de gerenciamento de processos (C + Assembly)
    │   │   ├── process.h    # Header do subsistema de processos
    │   │   ├── process.c    # Gerenciamento de processos
    │   │   ├── scheduler.c  # Implementação do scheduler
    │   │   └── switch.asm   # Troca de contexto
    │   ├── drivers/         # Drivers de dispositivo (C)
    │   │   ├── timer.c/.h   # Driver de timer
    │   │   ├── keyboard.c/.h  # Driver de teclado
    │   │   ├── mouse.c/.h   # Driver de mouse
    │   │   ├── serial.c/.h  # Driver de porta serial
    │   │   └── disk.c/.h    # Driver de disco
    │   └── rust/            # Biblioteca estática Rust no_std (v0.3+)
    │       ├── Cargo.toml          # Manifesto do workspace
    │       ├── rust-toolchain.toml # Versão nightly fixada
    │       ├── i686-unknown-none.json  # Especificação de target customizada
    │       ├── .cargo/config.toml  # Profile de build e rustflags
    │       ├── cbindgen.toml       # Config de geração de headers C
    │       ├── munux-rs/           # Crate principal (Rust seguro)
    │       │   └── src/
    │       │       ├── lib.rs      # Raiz da crate (no_std)
    │       │       ├── heap.rs     # Alocador de heap (first-fit + coalesce)
    │       │       └── sync.rs     # IrqMutex<T> (spin-lock + cli/sti)
    │       ├── munux-rs-ffi/       # Crate de shim FFI (todo o unsafe vive aqui)
    │       │   └── src/
    │       │       ├── lib.rs      # Exportações extern "C"
    │       │       └── panic.rs    # #[panic_handler] roteado ao panic do kernel
    │       └── include/            # Headers C gerados (versionados)
    │           └── munux_rs.h
    └── build/               # Saídas de build (geradas, no gitignore)
        ├── *.o              # Arquivos-objeto (C e Assembly)
        ├── libmunux_rs.a    # Biblioteca estática Rust
        ├── kernel.elf       # Executável ELF do kernel (link final)
        ├── kernel.bin       # Binário flat do kernel
        ├── bootloader.bin   # Binário do bootloader
        └── munux.iso        # Imagem ISO inicializável
│
└── munux-zos/              # Flavor 2: back-end transacional inspirado em mainframe (ESQUELETO)
    ├── README.md            # Visão e propósito do flavor z/OS
    ├── docs/                # Documentação SÓ do munux-zos
    │   ├── ROADMAP.md       # Roadmap do z/OS
    │   ├── ARCHITECTURE.md  # Arquitetura interna (JES/JCL/Datasets)
    │   └── PROTOCOL.md      # MJP — protocolo da ponte de integração
    ├── boot/                # (esqueleto — .gitkeep)
    └── kernel/              # (esqueleto — .gitkeep)
```

## Propósito dos Diretórios

### Diretório Raiz

**AUTHORSHIP.md**: Declara a autoria do projeto por Munique Feitoza, com timestamp e informações de licenciamento.

**LICENSE**: Texto completo da GNU General Public License v3.0, sob a qual o Munux é distribuído.

**README.md**: Visão geral de alto nível do projeto, cobrindo visão, objetivos, filosofia e funcionalidades planejadas.

### docs/

Contém toda a documentação do projeto escrita em formato Markdown. Cada documento cobre em profundidade um aspecto específico do sistema.

A documentação é escrita para múltiplos públicos:
- Estudantes que buscam entender conceitos de sistemas operacionais
- Desenvolvedores que contribuem para o Munux
- Arquitetos de sistemas que estudam decisões de projeto

Toda a documentação faz referências cruzadas a documentos relacionados e ao código-fonte.

### munux-os/

A própria implementação do kernel. Todo o código-fonte do bootloader e do kernel reside aqui.

Organizado por subsistema para manter uma separação clara de responsabilidades e permitir o desenvolvimento independente de cada componente.

### munux-os/boot/

Código do bootloader escrito em assembly x86. Responsável por:
- Carregar o kernel do disco para a memória
- Fazer a transição do modo real de 16 bits para o modo protegido de 32 bits
- Configurar a GDT inicial
- Transferir o controle para o kernel

**bootloader.asm**: Lógica principal do bootloader, incluindo I/O de disco e carregamento do kernel

**pmode.asm**: Código de inicialização do modo protegido (atualmente integrado ao bootloader.asm)

### munux-os/kernel/

Código-fonte principal do kernel em C e assembly. Contém:

**kernel.c/h**: Ponto de entrada e sequência de inicialização de todos os subsistemas

**kernel.ld**: Linker script que controla o layout de memória do binário do kernel

### munux-os/kernel/interrupts/

Subsistema de tratamento de interrupções. Gerencia todas as exceções de CPU e interrupções de hardware.

**idt.c/h**: Gerenciamento e inicialização da Interrupt Descriptor Table

**interrupt.asm**: Stubs em assembly para cada vetor de interrupção (macros geram os handlers ISR/IRQ)

**io.h**: Funções inline para operações de I/O de porta (inb, outb, inw, outw, etc.)

### munux-os/kernel/memory/

Subsistema de gerenciamento de memória que implementa uma abstração de memória em três camadas.

**memory.h**: API pública de todas as funções de memória

**pmm.c**: Physical Memory Manager — alocação de frames via bitmap

**vmm.c**: Virtual Memory Manager — paginação com page directory e page tables

**heap.c**: Heap allocator — implementação de malloc/free usando o algoritmo first-fit

**utils.c**: Utilitários de manipulação de memória (memset, memcpy, memcmp)

### munux-os/kernel/process/

Subsistema de gerenciamento de processos e escalonamento.

**process.h**: API pública para operações de processo

**process.c**: Criação, término e gerenciamento de processos

**scheduler.c**: Scheduler round-robin com filas de prioridade

**switch.asm**: Troca de contexto em assembly para salvar/restaurar o estado da CPU

### munux-os/kernel/drivers/

Drivers de dispositivo para o hardware essencial.

Cada driver consiste em uma implementação .c e um header .h:

**timer**: Programmable Interval Timer para marcação de tempo e escalonamento

**keyboard**: Teclado PS/2 com tradução de scancode e buffering de entrada

**mouse**: Mouse PS/2 com rastreamento de movimento e estados dos botões

**serial**: Porta serial RS-232 para depuração e comunicação externa

**disk**: Controlador de disco ATA/IDE para acesso a armazenamento em massa

### munux-os/kernel/rust/

O subsistema Rust, introduzido na v0.3. Compila para uma biblioteca estática `no_std` (`libmunux_rs.a`), que é linkada ao ELF do kernel junto com os objetos de C e Assembly.

**Cargo.toml**: Manifesto do workspace que declara as crates membras `munux-rs` e `munux-rs-ffi`, além das configurações de profile compartilhadas.

**rust-toolchain.toml**: Fixa a versão exata do Rust nightly para que todo contribuidor produza artefatos bit-a-bit idênticos.

**i686-unknown-none.json**: Especificação de target customizada — x86 bare-metal de 32 bits, ABI System V, sem float por hardware, modelo de relocação `static`.

**.cargo/config.toml**: Define a flag `--target` padrão e os `rustflags` para que o `cargo build` funcione sem argumentos de linha de comando.

**cbindgen.toml**: Configuração do gerador de headers `cbindgen`, que emite `include/munux_rs.h` a partir da API pública do Rust.

**munux-rs/**: A crate principal, contendo o código Rust seguro. Proíbe blocos unsafe, exceto exceções bem justificadas e auditadas.

**munux-rs-ffi/**: A crate de shim FFI. Todas as exportações `extern "C"` e definições de `#[panic_handler]` residem aqui. Esta crate é o único lugar onde código unsafe é esperado.

**include/**: Headers C gerados, versionados no controle de versão. O kernel em C faz `#include` desses arquivos para chamar o Rust.

### munux-os/build/

Gerado durante a compilação. Contém os artefatos de build intermediários e finais.

**NÃO** é versionado no controle de versão — é recriado a cada build.

**arquivos .o**: Arquivos-objeto compilados para cada fonte .c e .asm

**kernel.elf**: Kernel linkado em formato ELF com símbolos de depuração

**kernel.bin**: Binário flat do kernel extraído do ELF para carregamento

**bootloader.bin**: Bootloader montado (exatamente 512 bytes com a assinatura 0xAA55)

**munux.iso**: Imagem ISO inicializável combinando bootloader e kernel

## Convenções de Nomenclatura de Arquivos

**Arquivos assembly**: extensão `.asm` (sintaxe NASM)

**Fonte C**: extensão `.c`

**Headers C**: extensão `.h` (escritos à mão em `kernel/`; gerados pelo `cbindgen` em `kernel/rust/include/`)

**Fonte Rust**: extensão `.rs`, organizada em crates Cargo sob `kernel/rust/`

**Manifestos Cargo**: `Cargo.toml`

**Docs Markdown**: extensão `.md`

**Scripts de build**: `Makefile` (sem extensão)

**Linker scripts**: extensão `.ld`

**Especificações de target**: extensão `.json` (targets customizados do Rust)

## Princípios de Organização do Código

### Separação de Responsabilidades

Cada subsistema é independente, com interfaces bem definidas. O gerenciamento de memória não precisa conhecer os detalhes internos de processos. Os drivers não dependem dos detalhes de escalonamento.

### Arquitetura em Camadas

Camadas superiores se apoiam nas camadas inferiores:
- Camada 0: Hardware (CPU, dispositivos)
- Camada 1: Drivers e handlers de interrupção
- Camada 2: Gerenciamento de memória e de processos
- Camada 3: Serviços de sistema (futuro: VFS, syscalls)
- Camada 4: Espaço de usuário (futuro)

### Arquivos de Header

Os headers declaram APIs públicas e estruturas de dados. Os detalhes de implementação permanecem nos arquivos .c.

Os headers usam include guards para evitar inclusão múltipla:
```c
#ifndef SUBSYSTEM_H
#define SUBSYSTEM_H
// declarações
#endif
```

### Integração com Assembly

O código assembly faz interface com o C por meio de protótipos declarados:

O C declara: `extern void switch_to_process(...);`

O assembly define: `global switch_to_process`

Isso permite uma integração transparente, mantendo o código crítico de performance em assembly.

## Organização do Sistema de Build

### Estrutura do Makefile

A seção de variáveis define ferramentas e flags

As pattern rules compilam fontes em objetos

As regras explícitas tratam casos especiais (bootloader, linking)

Os phony targets fornecem comandos ao usuário (all, clean, run)

### Fases de Compilação

1. Montar o bootloader em binário flat
2. Compilar as fontes C em objetos ELF
3. Montar as fontes ASM em objetos ELF
4. Buildar o workspace Rust (`cargo build --release`) produzindo `libmunux_rs.a`
5. Linkar todos os objetos C/ASM junto com a biblioteca estática Rust no ELF do kernel
6. Extrair o binário flat do ELF
7. Combinar em uma ISO inicializável

As fases 2, 3 e 4 são independentes e paralelizam de forma limpa sob `make -j`.

### Rastreamento de Dependências

O Make rastreia dependências automaticamente por meio das inclusões de header. Alterar um header recompila todas as fontes que o incluem.

## Fluxo de Desenvolvimento

### Adicionando Novas Funcionalidades

1. Planeje a funcionalidade e identifique os subsistemas afetados
2. Crie ou modifique headers com as novas APIs
3. Implemente a funcionalidade em arquivos .c ou .asm
4. Atualize o Makefile se estiver adicionando novos arquivos-fonte
5. Teste exaustivamente no QEMU
6. Atualize a documentação relevante
7. Faça commit das alterações com uma mensagem descritiva

### Modificando Código Existente

1. Entenda a implementação atual (leia o código e a documentação)
2. Faça alterações pontuais, preservando as interfaces existentes quando possível
3. Atualize a documentação para refletir as alterações
4. Teste para garantir que não há regressões
5. Faça commit com uma explicação das alterações

### Depurando Problemas

1. Reproduza o problema de forma confiável
2. Adicione saída de depuração serial para restringir a localização
3. Use o GDB para inspecionar o estado, se necessário
4. Corrija a causa raiz, não os sintomas
5. Adicione verificações para evitar problemas semelhantes
6. Documente a correção se ela não for óbvia

## Manutenção da Documentação

A documentação deve permanecer sincronizada com o código:

- Atualize a documentação ao alterar comportamentos
- Adicione documentação para novas funcionalidades
- Remova a documentação de funcionalidades excluídas
- Mantenha os exemplos precisos e funcionais

Uma boa documentação é tão importante quanto um bom código para um SO educacional.

## Padrões de Qualidade

### Qualidade do Código

- Nomes claros e descritivos para funções e variáveis
- Comentários que explicam o "porquê", não o "o quê"
- Indentação e formatação consistentes
- Nenhuma complexidade desnecessária
- Trate os erros com elegância

### Qualidade da Documentação

- Reflexo preciso da implementação atual
- Organizada logicamente, com hierarquia clara
- Exemplos que compilam e funcionam
- Referências cruzadas para informações relacionadas
- Nível de detalhe apropriado para o público-alvo

### Qualidade dos Testes

- Inicializar e rodar com sucesso no QEMU
- Sem crashes ou panics em operação normal
- Tratamento adequado de erros para condições anormais
- Memória não corrompida nem vazada
- Performance aceitável para o uso pretendido

## Organização Futura

À medida que o Munux cresce, diretórios adicionais serão incluídos:

**userspace/**: Programas e bibliotecas de modo usuário

**tools/**: Utilitários de build e desenvolvimento

**tests/**: Suítes de testes automatizados

**fs/**: Implementações de sistema de arquivos

**net/**: Stack de rede

A estrutura atual fornece uma base sólida para esse crescimento, mantendo a clareza e a organização.

---

**Veja Também**:
- [munux-os/docs/BUILD.md](../munux-os/docs/BUILD.md) para detalhes do sistema de build
- [munux-os/docs/ARCHITECTURE.md](../munux-os/docs/ARCHITECTURE.md) para uma visão geral do design
- Código-fonte para detalhes de implementação
