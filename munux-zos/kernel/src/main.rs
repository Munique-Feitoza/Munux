//! Munux z/OS — kernel bare-metal para s390x (z/Architecture).
//!
//! Fase 1: bring-up mínimo que imprime um banner no console via **SCLP**
//! (Service-Call Logical Processor). O firmware `s390-ccw` do QEMU carrega o
//! kernel em 0x10000 (ver o linker script) e entrega o controle a `start`, em
//! z/Architecture 64-bit.
//!
//! Console (fielmente ao bios s390-ccw do QEMU): WRITE EVENT MASK habilita o
//! console ASCII, e cada WRITE EVENT DATA envia uma string, ambos pela
//! instrução `servc`. Após o `servc`, esperamos o **sinal de serviço**
//! (interrupção externa 0x2401) via um handler em low-core — correto também em
//! hardware real, não só no QEMU. Ao terminar, a CPU entra em **disabled-wait**.

#![no_std]
#![no_main]

use core::panic::PanicInfo;

// ---------------------------------------------------------------------------
// Entry + baixo nível (Assembly): entrada, Service Call + espera, handler da
// interrupção externa, disabled-wait, e as PSWs constantes.
//
// Valores de PSW e a sequência do handler vêm do bios `s390-ccw` do QEMU.
// ---------------------------------------------------------------------------
core::arch::global_asm!(
    ".section .init",
    ".globl start",
    "start:",
    "    larl  %r15, stackptr",      // pilha (topo, do linker)
    "    aghi  %r15, -160",          // área de save de 160 bytes (ABI s390x)
    "    brasl %r14, sclp_int_init", // instala o PSW novo de interrupção externa
    "    brasl %r14, kmain",         // kernel em Rust (SCLP setup + banner)
    "    brasl %r14, disabled_wait", // para a CPU de forma limpa (não retorna)
    ".section .text",
    // Instala em low-core o PSW novo de interrupção externa (mask em 0x1b0,
    // endereço do handler em 0x1b8).
    ".globl sclp_int_init",
    "sclp_int_init:",
    "    larl  %r1, external_new_code",
    "    stg   %r1, 0x1b8",
    "    larl  %r1, external_new_mask",
    "    mvc   0x1b0(8), 0(%r1)",
    "    br    %r14",
    // Emite um Service Call (R2=comando, R3=endereço do SCCB), devolve o
    // condition code em R2, e espera o sinal de serviço. O handler externo
    // retorna via `br %r14` direto para o chamador desta função.
    ".globl sclp_service_call",
    "sclp_service_call:",
    "    .insn rre, 0xb2200000, %r2, %r3", // servc
    "    ipm   %r2",
    "    srl   %r2, 28", // R2 = cc (valor de retorno)
    "    larl  %r1, cr0_scratch",
    "    stctg %c0, %c0, 0(%r1)",
    "    oi    6(%r1), 0x02", // habilita subclasse "service signal" no CR0
    "    lctlg %c0, %c0, 0(%r1)",
    "    larl  %r1, enabled_wait_psw",
    "    lpswe 0(%r1)", // enabled-wait: aguarda o sinal de serviço
    // (inalcançável: o handler faz br %r14 e volta ao chamador)

    // Handler da interrupção externa: desabilita a subclasse de serviço no CR0
    // e retorna ao chamador de sclp_service_call (r14 preservado pela interrupção).
    "external_new_code:",
    "    larl  %r1, cr0_scratch",
    "    stctg %c0, %c0, 0(%r1)",
    "    ni    6(%r1), 0xfd", // desabilita subclasse "service signal"
    "    lctlg %c0, %c0, 0(%r1)",
    "    br    %r14",
    // Para a CPU: carrega um disabled-wait PSW.
    ".globl disabled_wait",
    "disabled_wait:",
    "    larl  %r1, disabled_wait_psw",
    "    lpswe 0(%r1)",
    "1:  j 1b",
    ".section .data",
    ".align 8",
    "external_new_mask:",
    "    .quad 0x0000000180000000", // 64-bit, DAT off, interrupções off
    "enabled_wait_psw:",
    "    .quad 0x0302000180000000", // wait + I/O + externa habilitadas, 64-bit
    "    .quad 0x0000000000000000",
    "disabled_wait_psw:",
    "    .quad 0x0002000180000000", // wait, tudo desabilitado, 64-bit
    "    .quad 0x0000000000000000",
    "cr0_scratch:",
    "    .quad 0",
);

extern "C" {
    /// Emite um Service Call e espera a conclusão. Devolve o condition code.
    fn sclp_service_call(command: u64, sccb: u64) -> u64;
}

// ---------------------------------------------------------------------------
// SCLP: console ASCII
// ---------------------------------------------------------------------------

const SCCB_SIZE: usize = 4096;

const SCLP_CMD_WRITE_EVENT_DATA: u64 = 0x0076_0005;
const SCLP_CMD_WRITE_EVENT_MASK: u64 = 0x0078_0005;
const SCLP_EVENT_ASCII_CONSOLE_DATA: u8 = 0x1a;
const SCLP_EVENT_MASK_MSG_ASCII: u32 = 0x0000_0040;
const SCLP_FC_NORMAL_WRITE: u8 = 0;

/// Cabeçalho do SCCB (8 bytes). Campos naturalmente alinhados; num alvo
/// big-endian o `#[repr(C)]` já produz o byte order do mainframe.
#[repr(C)]
struct SccbHeader {
    length: u16,
    function_code: u8,
    control_mask: [u8; 3],
    response_code: u16,
}

/// Cabeçalho de buffer de evento (6 bytes).
#[repr(C)]
struct EventBufferHeader {
    length: u16,
    kind: u8,
    flags: u8,
    reserved: u16,
}

#[repr(C)]
struct WriteEventMask {
    h: SccbHeader,
    reserved: u16,
    mask_length: u16,
    cp_receive_mask: u32,
    cp_send_mask: u32,
    send_mask: u32,
    receive_mask: u32,
}

#[repr(C)]
struct WriteEventData {
    h: SccbHeader,
    ebh: EventBufferHeader,
    // os bytes do texto seguem imediatamente após este cabeçalho.
}

/// SCCB estático, alinhado a 4 KiB (exigência do Service Call).
#[repr(C, align(4096))]
struct Sccb([u8; SCCB_SIZE]);
static mut SCCB: Sccb = Sccb([0; SCCB_SIZE]);

fn sccb_ptr() -> *mut u8 {
    &raw mut SCCB as *mut u8
}

/// Habilita o console ASCII (WRITE EVENT MASK: cp_send_mask = ASCII).
unsafe fn sclp_setup() {
    let base = sccb_ptr();
    core::ptr::write_bytes(base, 0, SCCB_SIZE);
    let m = base.cast::<WriteEventMask>();
    (*m).h.length = core::mem::size_of::<WriteEventMask>() as u16;
    (*m).mask_length = core::mem::size_of::<u32>() as u16;
    (*m).cp_receive_mask = 0;
    (*m).cp_send_mask = SCLP_EVENT_MASK_MSG_ASCII;
    sclp_service_call(SCLP_CMD_WRITE_EVENT_MASK, base as u64);
}

/// Escreve uma string ASCII no console (WRITE EVENT DATA), convertendo
/// `\n` em `\r\n` como o bios faz.
unsafe fn sclp_write(s: &[u8]) {
    let base = sccb_ptr();
    core::ptr::write_bytes(base, 0, SCCB_SIZE);
    let hdr_len = core::mem::size_of::<WriteEventData>();
    let data = base.add(hdr_len);
    let cap = SCCB_SIZE - hdr_len;

    let mut n = 0usize;
    for &b in s {
        if n + 2 >= cap {
            break;
        }
        if b == b'\n' {
            *data.add(n) = b'\r';
            n += 1;
        }
        *data.add(n) = b;
        n += 1;
    }

    let d = base.cast::<WriteEventData>();
    (*d).h.length = (hdr_len + n) as u16;
    (*d).h.function_code = SCLP_FC_NORMAL_WRITE;
    (*d).ebh.length = (core::mem::size_of::<EventBufferHeader>() + n) as u16;
    (*d).ebh.kind = SCLP_EVENT_ASCII_CONSOLE_DATA;
    (*d).ebh.flags = 0;
    sclp_service_call(SCLP_CMD_WRITE_EVENT_DATA, base as u64);
}

/// Kernel principal (chamado por `start`; retorna para o disabled-wait).
#[no_mangle]
extern "C" fn kmain() {
    // SAFETY: single-CPU, sem preempção; o SCCB estático é usado só aqui.
    unsafe {
        sclp_setup();
        sclp_write(b"\n");
        sclp_write(b"========================================\n");
        sclp_write(b"   Munux z/OS  -  s390x / z-Architecture\n");
        sclp_write(b"   hello, mainframe!\n");
        sclp_write(b"========================================\n");
        sclp_write(b"\n[fase 1] boot + console SCLP: OK\n");
    }
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    loop {}
}
