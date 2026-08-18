//! JES — Job Entry Subsystem (Fase 2).
//!
//! Modela os conceitos de mainframe do processamento em lote:
//! - **Address spaces:** um espaço de endereçamento por job (aqui como
//!   estrutura + ASID; a isolação por hardware via DAT é hardening futuro).
//! - **Escalonamento batch:** o dispatcher escolhe o job de maior prioridade
//!   na fila INPUT e o roda até o fim — sem preempção interativa.
//! - **Máquina de estados do job:** `INPUT → ACTIVE → OUTPUT → PURGE`.
//!
//! Sem heap: pools de tamanho fixo. Saída pelo console SCLP.

#![allow(clippy::cast_possible_truncation)]

use crate::console_write as w;
use crate::console_write_u32_pad as wpad;

const MAX_ASID: usize = 8;
const MAX_JOBS: usize = 16;

/// Estados pelos quais um job passa no seu ciclo de vida.
#[derive(Clone, Copy, PartialEq, Eq)]
enum JobState {
    Input,
    Active,
    Output,
    Purge,
}

/// Um espaço de endereçamento (ASID = Address Space IDentifier).
#[derive(Clone, Copy)]
struct AddressSpace {
    asid: u16,
    in_use: bool,
}

/// Um job em lote.
#[derive(Clone, Copy)]
struct Job {
    id: u32,       // número do job (JOBnnnnn)
    name: [u8; 8], // nome de 8 caracteres (estilo mainframe)
    class: u8,     // classe do job (A-Z)
    priority: u8,  // 0..15 — maior = mais prioritário
    state: JobState,
    asid: u16, // espaço de endereçamento atribuído
    steps: u8, // simulação de trabalho: número de passos
}

/// O subsistema de entrada de jobs: pool de address spaces + fila de jobs.
struct Jes {
    spaces: [AddressSpace; MAX_ASID],
    jobs: [Option<Job>; MAX_JOBS],
    next_job_no: u32,
}

impl Jes {
    fn new() -> Self {
        let mut spaces = [AddressSpace {
            asid: 0,
            in_use: false,
        }; MAX_ASID];
        for (i, s) in spaces.iter_mut().enumerate() {
            s.asid = i as u16 + 1; // ASIDs 1..=MAX_ASID (0 fica reservado)
        }
        Jes {
            spaces,
            jobs: [None; MAX_JOBS],
            next_job_no: 1,
        }
    }

    /// Submete um job (entra na fila INPUT). Devolve o número, ou 0 se cheio.
    fn submit(&mut self, name: &[u8; 8], class: u8, priority: u8, steps: u8) -> u32 {
        for slot in &mut self.jobs {
            if slot.is_none() {
                let id = self.next_job_no;
                self.next_job_no += 1;
                *slot = Some(Job {
                    id,
                    name: *name,
                    class,
                    priority,
                    state: JobState::Input,
                    asid: 0,
                    steps,
                });
                return id;
            }
        }
        0
    }

    /// Aloca o primeiro address space livre. Devolve o ASID, ou 0 se esgotado.
    fn alloc_asid(&mut self) -> u16 {
        for s in &mut self.spaces {
            if !s.in_use {
                s.in_use = true;
                return s.asid;
            }
        }
        0
    }

    fn free_asid(&mut self, asid: u16) {
        for s in &mut self.spaces {
            if s.asid == asid {
                s.in_use = false;
            }
        }
    }

    /// Índice do job INPUT de maior prioridade (batch, sem preempção).
    fn pick_next(&self) -> Option<usize> {
        let mut best: Option<usize> = None;
        for (i, slot) in self.jobs.iter().enumerate() {
            let Some(j) = slot else { continue };
            if j.state != JobState::Input {
                continue;
            }
            match best {
                Some(b) => {
                    if let Some(bj) = &self.jobs[b] {
                        if j.priority > bj.priority {
                            best = Some(i);
                        }
                    }
                }
                None => best = Some(i),
            }
        }
        best
    }

    /// Imprime o prefixo identificando o job: `JOBnnnnn NOME     cl X pri NN`.
    fn print_prefix(job: &Job) {
        w(b"JOB");
        wpad(job.id, 5);
        w(b" ");
        w(&job.name);
        w(b" cl ");
        w(&[job.class]);
        w(b" pri ");
        wpad(u32::from(job.priority), 2);
    }

    /// Roda um job até o fim, passando por todos os estados.
    fn dispatch(&mut self, idx: usize) {
        let asid = self.alloc_asid();

        // INPUT -> ACTIVE
        self.jobs[idx].as_mut().unwrap().asid = asid;
        self.jobs[idx].as_mut().unwrap().state = JobState::Active;
        let steps = self.jobs[idx].as_ref().unwrap().steps;

        Self::print_prefix(self.jobs[idx].as_ref().unwrap());
        w(b" asid ");
        wpad(u32::from(asid), 2);
        w(b": INPUT -> ACTIVE\n");

        // Execução (simulada): imprime cada passo do job.
        for s in 1..=steps {
            w(b"    step ");
            wpad(u32::from(s), 1);
            w(b"/");
            wpad(u32::from(steps), 1);
            w(b" ...\n");
        }

        // ACTIVE -> OUTPUT (SYSOUT pronto)
        self.jobs[idx].as_mut().unwrap().state = JobState::Output;
        Self::print_prefix(self.jobs[idx].as_ref().unwrap());
        w(b": ACTIVE -> OUTPUT (SYSOUT pronto)\n");

        // OUTPUT -> PURGE (libera o address space, remove o job)
        self.jobs[idx].as_mut().unwrap().state = JobState::Purge;
        Self::print_prefix(self.jobs[idx].as_ref().unwrap());
        w(b": OUTPUT -> PURGE (asid ");
        wpad(u32::from(asid), 2);
        w(b" liberado)\n\n");

        self.free_asid(asid);
        self.jobs[idx] = None;
    }

    /// Processa toda a fila, em lote, por ordem de prioridade.
    fn run(&mut self) {
        while let Some(idx) = self.pick_next() {
            self.dispatch(idx);
        }
    }
}

/// Demonstração da Fase 2: submete alguns jobs e roda o escalonador.
pub fn run_demo() {
    let mut jes = Jes::new();
    jes.submit(b"PAYROLL ", b'A', 9, 3);
    jes.submit(b"BACKUP  ", b'B', 3, 2);
    jes.submit(b"REPORTS ", b'A', 6, 2);

    w(b"\n[JES] 3 jobs submetidos (fila INPUT).\n");
    w(b"[JES] Escalonamento batch por prioridade (maior primeiro):\n\n");
    jes.run();
    w(b"[fase 2] address spaces + escalonamento + estados: OK\n");
}
