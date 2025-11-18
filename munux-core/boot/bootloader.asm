; Bootloader básico em Assembly - Munux Core
; Carregado pela BIOS na memória em 0x7C00 (endereço padrão para bootloader)

BITS 16                  ; Modo real (16bits)
ORG 0x7C00               ; Define o ponto inicial da execução do bootloader na memória

; As mensagens que serão mostradas na tela
msg db "Iniciando Munux OS...", 13, 10, 0
loading_msg db "Carregando kernel...", 13, 10, 0
success_msg db "Kernel carregado com sucesso!", 13, 10, 0
disk_error_msg db "Erro ao carregar kernel!", 13, 10, 0

start:
    cli                  ; Desativa interrupções
    xor ax, ax           ; Zera o registrador AX
    mov ds, ax           ; Configura o segmento de dados (DS) para 0x0000
    mov es, ax           ; Configura o segmento extra (ES) para 0x0000
    mov ss, ax           ; Configura o segmento de pilha (SS) para 0x0000
    mov sp, 0x7C00       ; Define o ponteiro da pilha (SP) no topo do setor de boot (0x7C00)
    sti                  ; Ativa interrupções

    ; Limpar a tela
    call clear_screen

    ; Mensagem: "Iniciando Munux OS..."
    mov si, msg
    call print_string
    
    ; Mensagem: "Carregando kernel..."
    mov si, loading_msg
    call print_string
    
    ; Carregar o kernel
    call load_kernel
    
    ; Mensagem de sucesso
    mov si, success_msg
    call print_string
    
    ; Pequena pausa antes de continuar
    call delay
    
    ; Entrar em modo protegido
    jmp enter_pm

; Função para limpar a tela
clear_screen:
    pusha               ; Salva todos os registradores
    mov ah, 0x07        ; Função scroll up
    mov al, 0           ; Limpar tela inteira
    mov bh, 0x07        ; Atributo (cinza claro sobre preto)
    mov cx, 0           ; Canto superior esquerdo
    mov dx, 0x184F      ; Canto inferior direito (24,79)
    int 0x10           ; Chama interrupção BIOS
    
    ; Posicionar cursor no início
    mov ah, 0x02        ; Função set cursor position
    mov bh, 0           ; Página 0
    mov dx, 0           ; Linha 0, coluna 0
    int 0x10           ; Chama interrupção BIOS
    popa               ; Restaura registradores
    ret

; Função para imprimir string
print_string:
    pusha              ; Salva todos os registradores
print_loop:
    lodsb              ; Carrega próximo caractere
    or al, al          ; Verifica se é NULL
    jz print_done      ; Se zero, termina
    mov ah, 0x0E       ; Função teletipo
    mov bh, 0          ; Página 0
    mov bl, 0x07       ; Cor cinza claro
    int 0x10           ; Chama BIOS
    jmp print_loop     ; Continua loop
print_done:
    popa               ; Restaura registradores
    ret

; Função para adicionar delay
delay:
    pusha
    mov cx, 0xFFFF
delay_loop:
    nop
    loop delay_loop
    popa
    ret

; Carrega o kernel do disco
; Lê 15 setores a partir do setor 2 (imediatamente após o bootloader)
; Armazena em 0x80000 (512KB), que é um endereço dentro do primeiro megabyte
load_kernel:
    pusha                    ; Salva todos os registradores
    
    mov bx, 0x0000          ; Offset inicial
    mov ax, 0x8000          ; Segmento de destino
    mov es, ax              ; ES = 0x8000 (endereço 0x80000)
    
    mov ah, 0x02            ; Função BIOS: ler setores
    mov al, 15              ; Número de setores a ler
    mov ch, 0               ; Cilindro 0
    mov cl, 2               ; Setor inicial 2
    mov dh, 0               ; Cabeça 0
    mov dl, 0x80            ; Drive (primeiro disco rígido)
    
    int 0x13                ; Chama BIOS
    jc disk_error           ; Se erro, trata
    
    popa                    ; Restaura registradores
    ret

disk_error:
    mov si, disk_error_msg  ; Carrega mensagem de erro
    call print_string       ; Imprime mensagem
    jmp hang                ; Trava sistema

hang:
    jmp hang              ; Loop infinito para travar o sistema

; Rotina para entrar em modo protegido
enter_pm:
    cli                   ; Desliga interrupções

    lgdt [gdt_descriptor] ; Carrega o endereço e tamanho da tabela GDT

    jmp 0x08:init_pm  ; Jump para atualizar CS e entrar no modo protegido

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

; Definição da Global Descriptor Table (GDT)
gdt_start:
    dq 0x0000000000000000 ; Descriptor nulo obrigatório
    dq 0x00CF9A000000FFFF ; Descriptor de código (base=0, limite=4GB, executável, legível)
    dq 0x00CF92000000FFFF ; Descriptor de dados (base=0, limite=4GB, gravável)
gdt_end:

; Descriptor da GDT para o registrador GDTR (tamanho e endereço)
gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Tamanho da tabela GDT menos 1
    dd gdt_start                ; Endereço base da tabela GDT

; Constantes para segmentos da GDT
CODE_SEG equ 0x08
DATA_SEG equ 0x10

; Preenche o restante do setor com zeros até o byte 510
times 510-($-$$) db 0

; Assinatura mágica do bootloader (últimos 2 bytes do setor)
dw 0xAA55

