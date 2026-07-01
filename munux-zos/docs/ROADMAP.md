# Roadmap — Munux z/OS

> Flavor **inspirado em mainframe** do ecossistema Munux. Status atual: **esqueleto**
> (documentação + roadmap; **sem código de kernel**). Por design, **tudo está `[ ]`** —
> nada foi construído ainda; isto é um plano de projeto, não uma lista de recursos prontos.
> Legenda: **`[x]`** feito e validado · **`[ ]`** a fazer.
> Endgame do ecossistema: virar o **servidor MJP** que o [munux-os](../../munux-os/)
> (cliente) usa para submeter jobs — ver a fase 🌉.

## Panorama

|    Fase    | Nome                                   | Estado                                |
| :---------: | -------------------------------------- | ------------------------------------- |
| **0** | Decisão de arquitetura                | 🔴 pendente —**bloqueia tudo** |
| **1** | Boot mínimo + console                 | ⬜ a fazer                            |
| **2** | Address Spaces + escalonamento de jobs | ⬜ a fazer                            |
| **3** | JES: fila de jobs + spool              | ⬜ a fazer                            |
| **4** | JCL: parser de job control             | ⬜ a fazer                            |
| **5** | Datasets (PS/PDS) + catálogo          | ⬜ a fazer                            |
| **6** | ENQ/DEQ (serialização)               | ⬜ a fazer                            |
| **7** | 🌉 Ponte MJP (servidor)                | ⬜ a fazer                            |
| **8** | SMF (auditoria)                        | ⬜ futuro                             |

---

## Fase 0 — Decisão de arquitetura 🔴 (pré-requisito de tudo)

> Nenhuma linha de kernel deve ser escrita antes de fixar o alvo. As opções são
> mutuamente exclusivas e mudam todo o toolchain. Registrar a escolha em
> [ARCHITECTURE.md](ARCHITECTURE.md).

- [ ] **Decidir:** (A) s390x autêntico *(z/Architecture, `qemu-system-s390x`, novo target
  Rust, IPL, ELF64 big-endian — hoje o emulador não está instalado)* **vs** (B) base x86
  compartilhada *(reaproveita PMM/VMM/heap/IDT do munux-os; roda no `qemu-system-i386`)*
- [ ] Documentar o racional da escolha e suas consequências de toolchain

## Fase 1 — Boot mínimo + console

- [ ] Bootstrap para a arquitetura escolhida
- [ ] Console de operador (equivalente ao SYSLOG)
- [ ] `kernel_main` que inicializa e imprime o banner ("hello mainframe")

## Fase 2 — Address Spaces + escalonamento de jobs

- [ ] Estrutura de *address space* (um espaço virtual por job)
- [ ] Escalonador orientado a lote (por classe/prioridade de job, sem preempção interativa)
- [ ] Máquina de estados do job: `INPUT → ACTIVE → OUTPUT → PURGE`

## Fase 3 — JES (Job Entry Subsystem)

- [ ] Fila de entrada (INPUT queue)
- [ ] Área de spool (SYSIN/SYSOUT)
- [ ] Ciclo de vida do job orquestrado pelo JES
- [ ] Numeração de jobs (`JOBnnnnn`) + reader/writer de spool

## Fase 4 — JCL (Job Control Language)

- [ ] Parser dos statements essenciais: `//JOB`, `//EXEC`, `//DD`
- [ ] Resolução de *steps* + alocação de DDs por step
- [ ] Códigos de retorno por step (`COND=` mais adiante)

## Fase 5 — Datasets

- [ ] **PS** (Physical Sequential) — orientado a registro, não byte-stream
- [ ] **PDS** (Partitioned) — diretório + *members*
- [ ] **DSN** hierárquico + catálogo (DSN → localização física)
- [ ] Camada sobre o storage da arquitetura escolhida (Fase 0)

## Fase 6 — ENQ/DEQ (serialização)

- [ ] Primitivas de serialização global de recursos (inspiração: GRS)
- [ ] `ENQ` exclusivo/compartilhado por nome de recurso; `DEQ` no fim do job/step
- [ ] Prevenção determinística de race conditions entre jobs concorrentes

---

## 🌉 Fase 7 — Ponte MJP (servidor): a conexão com o munux-os

> O ponto alto do ecossistema. Este flavor vira o **back-end** que o munux-os (cliente)
> comanda. Contrato: [PROTOCOL.md](PROTOCOL.md). Casa com a fase 🌉 do
> [roadmap do munux-os](../../munux-os/docs/ROADMAP.md).

- [ ] Servidor MJP: aceitar `SUBMIT`, responder `ACK`/`NAK`
- [ ] Transmitir `STATUS` / `SYSOUT` / `RESULT` conforme o job progride
- [ ] Atender `QUERY` / `CANCEL` com autorização por sessão (anti-IDOR)
- [ ] Tradução **EBCDIC ↔ ASCII** na borda
- [ ] Validação Zero Trust de todo frame (versão, tipo, teto de tamanho, `length` vs lido)

## Fase 8 — SMF (auditoria)

- [ ] Registros de contabilização por job (CPU, I/O, tempo de fila)
- [ ] Base para relatórios e accounting

---

## Fora de escopo por enquanto

- [ ] Compatibilidade binária com programas z/OS reais
- [ ] VSAM completo (KSDS/ESDS/RRDS) — só depois de PS/PDS sólidos
- [ ] Sysplex / multi-sistema

> Nota de fidelidade: JES, JCL, PS/PDS/VSAM, GRS/ENQ, SMF e TSO são subsistemas **reais**
> do z/OS. Aqui são reimplementados do zero em escala didática — nenhuma linha vem da IBM.

