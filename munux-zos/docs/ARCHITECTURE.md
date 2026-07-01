# Arquitetura — Munux z/OS

> **Documento de projeto.** Descreve o alvo arquitetural do flavor mainframe. Nada
> aqui está implementado ainda — é o plano contra o qual o código será escrito.

## Princípio central

O `munux-zos` não é um SO interativo. É um **processador de jobs**: entra trabalho por
uma fila, é executado com isolamento e serialização rigorosos, e sai resultado por
outra fila. A disciplina de mainframe (integridade > latência) guia cada decisão.

## Camadas

```mermaid
flowchart TB
    subgraph CLIENTE["munux-os (cliente / painel)"]
        UI[Terminal reativo]
    end

    subgraph ZOS["munux-zos (servidor / back-end transacional)"]
        MJP[Ponte MJP<br/>servidor + EBCDIC/ASCII]
        JES[JES<br/>fila de jobs + spool]
        JCL[Parser JCL<br/>JOB / EXEC / DD]
        DISP[Dispatcher de jobs<br/>por classe/prioridade]
        AS[Address Spaces<br/>1 espaço virtual por job]
        DS[Camada de Datasets<br/>PS / PDS + catálogo]
        GRS[ENQ/DEQ<br/>serialização de recursos]
        SMF[SMF<br/>auditoria/accounting]
        DASD[(Storage / DASD)]
    end

    UI -- MJP --> MJP
    MJP --> JES
    JES --> JCL --> DISP --> AS
    AS --> DS --> DASD
    AS --> GRS
    AS --> SMF
    JES -- SYSOUT --> MJP -- MJP --> UI

    classDef bridge fill:#dea584,stroke:#7d3c98,color:#000
    class MJP bridge
```

## Componentes

### Ponte MJP (borda)
Servidor do [Munux Job Protocol](PROTOCOL.md). É o **único** ponto de entrada externo.
Faz enquadramento binário, tradução **EBCDIC ↔ ASCII** e valida todo input como hostil
(o cliente é _untrusted_). Ver seção de segurança.

### JES — Job Entry Subsystem
Mantém a **INPUT queue**, o **spool** (SYSIN/SYSOUT) e o ciclo de vida do job
(`INPUT → ACTIVE → OUTPUT → PURGE`). Atribui número de job (`JOBnnnnn`).

### Parser JCL
Lê os statements `//JOB`, `//EXEC`, `//DD` e monta o plano de _steps_ do job, com as
alocações de dataset (DD) que cada step enxerga.

### Dispatcher + Address Spaces
Cada job recebe um **address space** próprio (espaço de endereçamento virtual isolado —
o análogo pesado de um processo). O dispatch é orientado a lote (por classe/prioridade
de job), não a time-sharing interativo.

### Camada de Datasets
Storage **orientado a registro** (não byte-stream): **PS** (sequencial) e **PDS**
(particionado, com _members_). Um **catálogo** resolve o **DSN** (dataset name
hierárquico) para a localização física. Assenta sobre o storage da arquitetura escolhida
(Fase 0).

### ENQ/DEQ (serialização)
Serialização global de recursos por nome (exclusivo/compartilhado), garantindo
concorrência **determinística** entre jobs — sem race conditions.

### SMF (auditoria)
Registros de contabilização por job (CPU, I/O, tempo de fila). Base de relatórios.

## Fronteira de confiança

```
[munux-os / rede]  ──untrusted──▶  [Ponte MJP]  ──validado──▶  [núcleo z/OS]
```

- Tudo que entra pela ponte é **hostil até prova em contrário**: valida tamanho de
  frame, versão, comprimentos declarados vs reais, e nunca confia em offsets do cliente.
- A tradução EBCDIC↔ASCII acontece na borda, isolando o núcleo de encoding externo.
- Isolamento por **address space** contém falha/abuso de um job sem derrubar o sistema.

## Decisão pendente (Fase 0)

A **arquitetura alvo** (s390x autêntico vs base x86 compartilhada) ainda não foi
fixada — ver [ROADMAP.md](ROADMAP.md). Toda a estrutura acima é **independente de
ISA**; só a implementação de boot, memória e storage muda conforme a escolha.
