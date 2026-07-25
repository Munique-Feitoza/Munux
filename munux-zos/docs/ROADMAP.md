# Roadmap — Munux z/OS

> Flavor **inspirado em mainframe** do ecossistema Munux. Status: **Fase 1 concluída** —
> boota no `qemu-system-s390x` (z/Architecture) e imprime "hello, mainframe!" pelo
> console SCLP. Legenda: **`[x]`** feito e validado · **`[ ]`** a fazer.
> Endgame do ecossistema: virar o **servidor MJP** que o [munux-os](../../munux-os/)
> (cliente) usa para submeter jobs — ver a fase 🌉.

## Panorama

|    Fase    | Nome                                   | Estado                                |
| :---------: | -------------------------------------- | ------------------------------------- |
| **0** | Decisão de arquitetura                | ✅ decidida: s390x autêntico          |
| **1** | Boot mínimo + console                 | ✅ boota + banner SCLP no QEMU        |
| **2** | Address Spaces + escalonamento de jobs | ⬜ a fazer                            |
| **3** | JES: fila de jobs + spool              | ⬜ a fazer                            |
| **4** | JCL: parser de job control             | ⬜ a fazer                            |
| **5** | Datasets (PS/PDS) + catálogo          | ⬜ a fazer                            |
| **6** | ENQ/DEQ (serialização)               | ⬜ a fazer                            |
| **7** | 🌉 Ponte MJP (servidor)                | ⬜ a fazer                            |
| **8** | SMF (auditoria)                        | ⬜ futuro                             |

---

## Fase 0 — Decisão de arquitetura ✅ (decidida: s390x autêntico)

> Decidido em 2026-07-01: alvo **s390x (z/Architecture)** — o mainframe de verdade,
> rodando no `qemu-system-s390x`. Racional e consequências de toolchain em
> [ARCHITECTURE.md](ARCHITECTURE.md). É um segundo kernel do zero: nada do munux-os
> (x86) é reaproveitado.

- [x] **Decisão:** s390x autêntico (z/Architecture, 64-bit, big-endian)
- [x] Consequências de toolchain documentadas *(verificado: não há target bare-metal
  s390x no Rust → precisa de `s390x-unknown-none.json` custom + `build-std`;
  `qemu-system-s390x` disponível no repo; boot pelo firmware `s390-ccw`)*

## Fase 1 — Boot mínimo + console (s390x) ✅

> **Verificado no `qemu-system-s390x` 11.0.2:** o kernel boota e imprime o banner
> "hello, mainframe!" pelo console SCLP. Fundamentado no bios `s390-ccw` do QEMU.

- [x] Target Rust custom `s390x-unknown-none.json` (`no_std`, 64-bit, big-endian) + `build-std`
  *(produz ELF `IBM S/390` MSB, entry `start` em 0x10000)*
- [x] Emulador instalado (`qemu-system-s390x` 11.0.2)
- [x] Entry `start` (monta stack + chama o kernel) carregado pelo firmware `s390-ccw` via `-kernel`
- [x] Console via **SCLP**: WRITE EVENT MASK habilita o console ASCII → WRITE EVENT DATA (`servc`) imprime o banner
- [x] Espera pelo **sinal de serviço** (interrupção externa `0x2401`) via handler em low-core — correto também em hardware real, não só no QEMU síncrono
- [x] **disabled-wait** para a CPU de forma limpa ao terminar *(QEMU sai sozinho em ~24 ms; só os 2 resets de power-on, sem exceções)*

> Gates: `cargo clippy` e `rustfmt --check` limpos.

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

