; Munux Kernel - ISR and IRQ Assembly Stubs
; 
; Stubs em Assembly para Interrupt Service Routines e Hardware IRQs

[BITS 32]

; Macro para ISRs sem código de erro
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        cli
        push byte 0         ; Push dummy error code
        push byte %1        ; Push interrupt number
        jmp isr_common_stub
%endmacro

; Macro para ISRs com código de erro
%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        cli
        push byte %1        ; Push interrupt number
        jmp isr_common_stub
%endmacro

; Macro para IRQs
%macro IRQ 2
    global irq%1
    irq%1:
        cli
        push byte 0         ; Push dummy error code
        push byte %2        ; Push interrupt number
        jmp irq_common_stub
%endmacro

; Exceções do processador (0-31)
ISR_NOERRCODE 0     ; Division By Zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; Non Maskable Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Into Detected Overflow
ISR_NOERRCODE 5     ; Out of Bounds
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; No Coprocessor
ISR_ERRCODE   8     ; Double Fault (pushes error code)
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Bad TSS (pushes error code)
ISR_ERRCODE   11    ; Segment Not Present (pushes error code)
ISR_ERRCODE   12    ; Stack Fault (pushes error code)
ISR_ERRCODE   13    ; General Protection Fault (pushes error code)
ISR_ERRCODE   14    ; Page Fault (pushes error code)
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 Floating Point Exception
ISR_ERRCODE   17    ; Alignment Check (pushes error code)
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating Point Exception
ISR_NOERRCODE 20    ; Virtualization Exception
ISR_NOERRCODE 21    ; Reserved
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Reserved
ISR_NOERRCODE 29    ; Reserved
ISR_ERRCODE   30    ; Security Exception (pushes error code)
ISR_NOERRCODE 31    ; Reserved

; Hardware IRQs (32-47)
IRQ 0,  32    ; Timer
IRQ 1,  33    ; Keyboard
IRQ 2,  34    ; Cascade
IRQ 3,  35    ; COM2
IRQ 4,  36    ; COM1
IRQ 5,  37    ; LPT2
IRQ 6,  38    ; Floppy
IRQ 7,  39    ; LPT1
IRQ 8,  40    ; CMOS RTC
IRQ 9,  41    ; Free
IRQ 10, 42    ; Free
IRQ 11, 43    ; Free
IRQ 12, 44    ; PS2 Mouse
IRQ 13, 45    ; FPU
IRQ 14, 46    ; Primary ATA
IRQ 15, 47    ; Secondary ATA

; Syscall (int 0x80). Não usa a macro IRQ: 0x80 tem o bit 7 setado e
; `push byte` faria sign-extension (viraria 0xFFFFFF80); empurramos dwords.
global isr128
isr128:
    cli
    push dword 0        ; dummy error code
    push dword 0x80     ; número da interrupção (128)
    jmp syscall_common_stub

; Funções externas em C
extern isr_handler
extern irq_handler
extern syscall_handler

; Stub comum para ISRs
isr_common_stub:
    pusha               ; Salva todos os registradores de propósito geral
    
    mov ax, ds          ; Salva segmento de dados
    push eax
    
    mov ax, 0x10        ; Carrega kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Passa registers_t* (topo aponta pro ds salvo)
    call isr_handler    ; Chama handler em C
    add esp, 4          ; Remove o argumento empurrado

    pop eax             ; Restaura segmento de dados original
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Restaura registradores
    add esp, 8          ; Limpa pushed error code e ISR number
    sti
    iret                ; Retorna da interrupção

; Stub comum para IRQs
irq_common_stub:
    pusha               ; Salva todos os registradores de propósito geral
    
    mov ax, ds          ; Salva segmento de dados
    push eax
    
    mov ax, 0x10        ; Carrega kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Passa registers_t* (topo aponta pro ds salvo)
    call irq_handler    ; Chama handler em C
    add esp, 4          ; Remove o argumento empurrado

    pop eax             ; Restaura segmento de dados original
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                ; Restaura registradores
    add esp, 8          ; Limpa pushed error code e IRQ number
    sti
    iret                ; Retorna da interrupção

; Stub comum para syscalls (int 0x80). Igual ao de IRQ, mas chama
; syscall_handler e NÃO envia EOI (não é interrupção de hardware). O handler
; pode alterar regs->eax; o popa restaura esse valor como retorno da syscall.
syscall_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp            ; registers_t*
    call syscall_handler
    add esp, 4

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8          ; limpa error code + número da interrupção
    sti
    iret

; Função para carregar a IDT
global idt_flush
idt_flush:
    mov eax, [esp+4]    ; Pega ponteiro para IDT
    lidt [eax]          ; Carrega IDT
    ret
