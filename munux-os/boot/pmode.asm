; Código em modo protegido (32 bits)
[BITS 32]

DATA_SEG equ 0x10


init_pm:
    ; Manipula o CR0 para ativar modo protegido
    mov eax, cr0          ; Lê registrador de controle CR0 para EAX
    or eax, 1             ; Seta o bit PE (Protection Enable) no CR0
    mov cr0, eax          ; Escreve o novo valor em CR0, ativando o modo protegido

    mov ax, DATA_SEG      ; Carrega seletor de segmento de dados na variável AX
    mov ds, ax            ; Atualiza DS para segmento de dados
    mov es, ax            ; Atualiza ES para segmento de dados
    mov fs, ax            ; Atualiza FS para segmento de dados
    mov gs, ax            ; Atualiza GS para segmento de dados
    mov ss, ax            ; Atualiza SS para segmento de dados

    jmp 0x8000:0x0000     ; Salta para kernel carregado em 0x80000
