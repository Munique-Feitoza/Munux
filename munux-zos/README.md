# Munux z/OS

> **Flavor 2 do ecossistema Munux** — o back-end transacional inspirado em mainframe.
> Para a visão do ecossistema e o flavor de propósito geral (`munux-os`), veja o [README raiz](../README.md).
>
> ⚠️ **Status: ESQUELETO.** Este flavor está em fase de projeto. Há documentação e
> roadmap; ainda **não existe código de kernel**. Os diretórios `boot/` e `kernel/`
> são _stubs_ (apenas `.gitkeep`).

---

## O que é

**Munux z/OS** é uma reimplementação **educacional e inspirada** nos conceitos de
mainframe da IBM z/OS — **não** é o z/OS da IBM e **não** usa nenhuma linha de código
dela. O objetivo é estudar e recriar, do zero, as ideias que tornam o mainframe único
na computação corporativa: processamento em lote (_batch_) massivo, integridade de
dados absoluta, serialização rigorosa de recursos e tolerância a falhas.

Enquanto o [`munux-os`](../munux-os/) é o **workspace reativo** (interativo, POSIX-like,
propósito geral), o `munux-zos` é o **processador transacional**: você **submete jobs**
e ele os executa com disciplina de mainframe.

## Por que separado do munux-os

Fundir um SO de propósito geral com um mainframe geraria um "Frankenstein conceitual"
que perderia o foco de ambos. A decisão arquitetural é mantê-los como **duas soluções
cirúrgicas** para problemas diferentes, **conectadas por uma ponte de alto nível**:

- **munux-os** → reativo, ágil, interação direta (terminal, drivers, scheduler).
- **munux-zos** → batch pesado, filas, _datasets_, concorrência determinística.

## Conceitos-alvo (todos inspirados em z/OS real)

| Conceito | Inspiração real (IBM z/OS) | Papel no Munux z/OS |
|---|---|---|
| **Address Spaces** | Cada job roda no próprio espaço de endereçamento virtual | Isolamento forte por job |
| **JES** — Job Entry Subsystem | JES2/JES3: fila de jobs, _spool_, SYSIN/SYSOUT | Recebe, enfileira e despacha jobs |
| **JCL** — Job Control Language | Statements `JOB` / `EXEC` / `DD` | Linguagem de controle de jobs |
| **Datasets** | Storage orientado a registro (PS, PDS, VSAM) | Armazenamento por DSN, não byte-stream |
| **ENQ/DEQ** (GRS) | Serialização global de recursos | Controle de concorrência determinístico |
| **SMF** | System Management Facilities — log/accounting | Auditoria e contabilização de jobs |

> Os nomes acima correspondem a subsistemas **reais** do z/OS. Aqui eles são
> reimplementados em escala didática — nenhuma linha vem da IBM.

## A ponte com o munux-os

O valor do ecossistema não está em fundir os dois mundos, e sim em **conectá-los**. O
`munux-os` atua como **cliente / painel de controle**, submetendo jobs e consumindo
resultados do `munux-zos` (**servidor / back-end**), através de um protocolo binário de
baixo nível — o **MJP (Munux Job Protocol)**.

Isso reproduz, em escala de estudo, a dor real do mercado corporativo: fazer sistemas
modernos conversarem, de forma performática e segura, com o _core_ transacional que
roda em mainframe.

- Arquitetura interna: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)
- Protocolo da ponte: [docs/PROTOCOL.md](docs/PROTOCOL.md)
- Visão do ecossistema: [../docs/ECOSYSTEM.md](../docs/ECOSYSTEM.md)

## Status e próximos passos

Veja o [ROADMAP.md](docs/ROADMAP.md). A **primeira decisão em aberto** é a _arquitetura
alvo_: **s390x autêntico** (z/Architecture, `qemu-system-s390x`) vs **base x86
compartilhada** com o munux-os. Nada de kernel é escrito antes dessa escolha.

## Licença e autoria

GPLv3, como todo o ecossistema Munux. Criado e mantido por **Munique Feitoza**.
