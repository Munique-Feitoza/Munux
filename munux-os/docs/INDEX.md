# Índice da Documentação do Munux

Bem-vindo à documentação do sistema operacional Munux. Este guia abrangente cobre cada aspecto da implementação do kernel, desde a arquitetura de alto nível até os detalhes de implementação de baixo nível.

> 📐 **Diagramas UML**: Todo documento de subsistema central agora vem acompanhado de diagramas UML baseados em Mermaid (componente, classe, sequência, estado e atividade). Eles são renderizados nativamente no GitHub — sem necessidade de ferramentas. Se você estiver lendo localmente, qualquer visualizador de Markdown que suporte Mermaid (VS Code com a extensão Markdown Preview Mermaid, Obsidian, Typora, etc.) irá exibi-los.

## Primeiros Passos

Novo no Munux? Comece por aqui:

1. **[README.md](../README.md)** — Visão geral e objetivos do projeto
2. **[BUILD.md](BUILD.md)** — Como compilar e executar o Munux (C, Assembly e Rust)
3. **[ARCHITECTURE.md](ARCHITECTURE.md)** — Design do sistema em alto nível
4. **[RUST.md](RUST.md)** — Estratégia de integração com Rust e a fronteira FFI

## Documentação Central

### Arquitetura do Sistema

**[ARCHITECTURE.md](ARCHITECTURE.md)**  
Visão geral abrangente da filosofia de design do Munux, da organização dos componentes e da arquitetura do sistema. Entenda como todas as peças se encaixam.

**[ROADMAP.md](ROADMAP.md)**  
Cronograma de desenvolvimento mostrando funcionalidades concluídas, trabalho em andamento e planos futuros. Veja para onde o Munux está indo.

**[RUST.md](RUST.md)**  
Estratégia para a adoção do Rust (v0.3): toolchain, especificação de target customizado, fronteira FFI, tratamento de panic, contrato do allocator e a ordem em que os subsistemas serão portados de C para Rust.

### Subsistemas

**[MEMORY.md](MEMORY.md)**  
Mergulho profundo no gerenciamento de memória: alocação de frames físicos, paginação de memória virtual e alocação de heap. Aprenda como o Munux gerencia um dos recursos mais críticos.

**[PROCESSES.md](PROCESSES.md)**  
Detalhes internos do gerenciamento de processos e do scheduling. Entenda como o Munux alcança a multitarefa por meio de blocos de controle de processo, algoritmos de scheduling e troca de contexto.

**[INTERRUPTS.md](INTERRUPTS.md)**  
O sistema de tratamento de interrupções forma a base da comunicação com o hardware. Aprenda sobre a IDT, os handlers de exceção, o gerenciamento de IRQ e o PIC.

**[DRIVERS.md](DRIVERS.md)**  
Arquitetura e implementações dos drivers de dispositivo. Cobre os drivers de timer, teclado, mouse, porta serial e disco, com explicações detalhadas de protocolo.

### Recursos de Desenvolvimento

**[BUILD.md](BUILD.md)**  
Guia completo para compilar, executar, testar e depurar o Munux. Inclui configuração da toolchain, um passo a passo do Makefile e dicas de solução de problemas.

**[API.md](API.md)**  
Referência abrangente da API do kernel. Cada função, estrutura e constante documentada com exemplos de uso.

## Mapa de Diagramas UML

Lista de acesso rápido aos diagramas adicionados ao longo da documentação:

| Documento | Diagramas |
|---|---|
| [ARCHITECTURE.md](ARCHITECTURE.md) | Diagrama de componentes (kernel ↔ hardware, com camada Rust) · Sequência de boot |
| [MEMORY.md](MEMORY.md) | Diagrama de classes (PMM/VMM/Heap) · Sequência de expansão do heap · Atividade de tradução de endereços |
| [PROCESSES.md](PROCESSES.md) | Diagrama de classes (PCB/Scheduler) · Máquina de estados de processo · Sequência de troca de contexto |
| [INTERRUPTS.md](INTERRUPTS.md) | Layout de vetores da IDT · Sequência do ciclo de vida da interrupção · Atividade de despacho de exceção/IRQ |
| [DRIVERS.md](DRIVERS.md) | Diagrama de componentes dos drivers · Sequência de IRQ do teclado |
| [RUST.md](RUST.md) | Diagrama da fronteira FFI · Sequência do pipeline de build · Progressão do port dos subsistemas |
| [README.md](../README.md) | Diagrama de visão geral do kernel (com as arestas FFI) |

## Documentação por Tópico

### Para Quem Está Aprendendo

Se você está aprendendo sistemas operacionais:

1. Comece com [ARCHITECTURE.md](ARCHITECTURE.md) para ter a visão geral
2. Leia [INTERRUPTS.md](INTERRUPTS.md) para entender a comunicação com o hardware
3. Estude [MEMORY.md](MEMORY.md) para os conceitos de gerenciamento de memória
4. Explore [PROCESSES.md](PROCESSES.md) para ver a multitarefa em ação
5. Revise [DRIVERS.md](DRIVERS.md) para exemplos reais de interação com hardware

### Para Desenvolvedores

Se você está contribuindo com o Munux:

1. Revise [BUILD.md](BUILD.md) para configurar seu ambiente
2. Consulte [API.md](API.md) para assinaturas de funções e uso
3. Verifique [ROADMAP.md](ROADMAP.md) para encontrar áreas que precisam de trabalho
4. Estude a documentação relevante dos subsistemas ([MEMORY.md](MEMORY.md), [PROCESSES.md](PROCESSES.md), etc.)
5. Siga os padrões de codificação demonstrados no código existente

### Para Arquitetos de Sistema

Se você está estudando design de SO:

1. [ARCHITECTURE.md](ARCHITECTURE.md) explica as decisões de design
2. [MEMORY.md](MEMORY.md) mostra o gerenciamento de memória em três camadas
3. [PROCESSES.md](PROCESSES.md) detalha os algoritmos de scheduling
4. [INTERRUPTS.md](INTERRUPTS.md) cobre a arquitetura de interrupções
5. [ROADMAP.md](ROADMAP.md) descreve os planos arquiteturais futuros

## Referência Rápida

### Tarefas Comuns

**Compilando o Munux**
```bash
cd munux-os
make
```
Veja [BUILD.md](BUILD.md) para detalhes.

**Executando no QEMU**
```bash
make run
```

**Depurando com o GDB**
```bash
make debug
# Em outro terminal:
gdb build/kernel.elf
(gdb) target remote localhost:1234
```

**Encontrando a documentação da API**
Veja [API.md](API.md) - organizada por subsistema com assinaturas completas.

### Conceitos-Chave

**Gerenciamento de Memória**
- Física: Alocação por bitmap de frames de 4KB
- Virtual: Paginação de dois níveis (PD + PT)
- Heap: Allocator first-fit com coalescência

**Scheduling de Processos**
- Algoritmo: Round-robin com prioridades
- Preempção: Baseada em quantum via interrupção do timer
- Troca de Contexto: Rotina em Assembly que salva/restaura o estado

**Tratamento de Interrupções**
- Exceções: Geradas pela CPU (divisão por zero, page fault, etc.)
- IRQs: Interrupções de hardware (timer, teclado, etc.)
- Fluxo: IDT → stub em Assembly → handler em C

## Padrões da Documentação

Toda a documentação do Munux segue estes princípios:

**Abrangente**: Cobrir todos os aspectos do tópico, da visão geral aos detalhes de implementação

**Precisa**: Refletir a implementação atual, atualizando quando o código muda

**Acessível**: Escrita para leitores com diferentes níveis de experiência

**Prática**: Incluir exemplos concretos e padrões de uso

**Bem Organizada**: Estrutura lógica com hierarquia clara

**Com Referências Cruzadas**: Vincular à documentação relacionada

## Contribuindo com a Documentação

Melhorias na documentação são sempre bem-vindas:

- Corrigir erros de digitação ou explicações confusas
- Adicionar detalhes ou exemplos que faltam
- Atualizar informações desatualizadas
- Criar novos guias ou tutoriais
- Melhorar a organização ou a navegação

## Estrutura da Documentação

```
docs/
├── INDEX.md           # Este arquivo — guia da documentação
├── ARCHITECTURE.md    # Visão geral da arquitetura do sistema
├── RUST.md            # Estratégia de integração com Rust e fronteira FFI
├── MEMORY.md          # Subsistema de gerenciamento de memória
├── PROCESSES.md       # Gerenciamento de processos e scheduling
├── INTERRUPTS.md      # Sistema de tratamento de interrupções
├── DRIVERS.md         # Drivers de dispositivo
├── BUILD.md           # Compilando e executando o Munux
├── API.md             # Referência completa da API
├── STRUCTURE.md       # Referência do layout do repositório
└── ROADMAP.md         # Roadmap de desenvolvimento
```

## Recursos Adicionais

### Código-Fonte

A documentação mais precisa é o próprio código:

```
munux-os/
├── kernel/           # Código principal do kernel
│   ├── interrupts/  # Tratamento de interrupções (C + Assembly)
│   ├── memory/      # Gerenciamento de memória (C; heap migrando para Rust)
│   ├── process/     # Gerenciamento de processos (C + Assembly)
│   ├── drivers/     # Drivers de dispositivo (C)
│   └── rust/        # Biblioteca estática Rust no_std (v0.3+)
├── boot/            # Bootloader (Assembly)
└── Makefile         # Sistema de build (orquestra GCC, NASM, Cargo, LD)
```

### Recursos Externos

Para entender a arquitetura x86 e conceitos de SO:

- Intel 64 and IA-32 Architectures Software Developer Manuals
- OSDev Wiki (osdev.org)
- "Operating Systems: Three Easy Pieces" por Remzi Arpaci-Dusseau
- "Modern Operating Systems" por Andrew Tanenbaum
- "The Design and Implementation of the FreeBSD Operating System"

## Obtendo Ajuda

Travado ou confuso? Tente:

1. Pesquisar nesta documentação usando a busca do seu editor
2. Ler em detalhe a documentação do subsistema relevante
3. Examinar a implementação no código-fonte
4. Consultar recursos externos para conceitos de base
5. Abrir uma issue no repositório

## Feedback

O feedback sobre a documentação é valioso:

- O que está confuso ou pouco claro?
- O que está faltando?
- Quais exemplos ajudariam?
- Como a organização pode melhorar?

Sua contribuição ajuda a tornar a documentação do Munux melhor para todos.

---

**Versão da Documentação**: 0.3  
**Última Atualização**: Corresponde à versão 0.3 do kernel (Adoção do Rust)  
**Mantido por**: Munique Feitoza  
**Licença**: GPLv3
