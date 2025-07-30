; Bootloader básico em Assembly - Munux Core
; Carregado pela BIOS na memória em 0x7C00 (endereço padrão para bootloader)

BITS 16                  ; Modo real (16bits)
ORG 0x7C00               ; Define o ponto inicial da execução do bootloader na memória

; As mensagens que serão mostrada na tela
msg db "Iniciando Munux...", 0
disk_error_msg db "Erro ao carregar kernel!", 0

start:
    cli                  ; Desativa interrupções
    xor ax, ax           ; Zera o registrador AX
    mov ds, ax           ; Configura o segmento de dados (DS) para 0x0000
    mov es, ax           ; Configura o segmento extra (ES) para 0x0000
    mov ss, ax           ; Configura o segmento de pilha (SS) para 0x0000
    mov sp, 0x7C00       ; Define o ponteiro da pilha (SP) no topo do setor de boot (0x7C00)
    sti                  ; Ativa interrupções

    ; Mensagem: "Iniciando Munux..."
    mov si, msg              ; Aponta o registrador SI para o início da mensagem

print_char:
    lodsb                ; Carrega o próximo byte da mensagem para AL e incrementa SI
    or al, al            ; Verifica se é NULL (fim da string)
    jz hang              ; Se zero, pula para loop infinito (fim da mensagem)
    mov ah, 0x0E         ; Modo de saida teletipo 
    int 0x10             ; Chama a interrupção da BIOS para imprimir caractere em AL
    jmp print_char       ; Loop para continuar imprimindo o próximo caractere

; Começa a função para carregar kernel do disco
; Lê 15 setores a partir do setor 2 (imediatamente após o bootloader)
; Armazena em 0x80000 (512KB), que é um endereço dentro do primeiro megabyte

load_kernel:
    mov bx, 0x0000
    mov ax, 0x8000
    mov es, ax           ; Segmento ES para 0x8000 (0x8000 * 16 = 0x80000)
    mov di, 0x0000       ; Offset DI para 0 (início do buffer)
    mov cx, 15           ; Número de setores a ler
    mov ch, 0            ; Cilindro 0
    mov cl, 2            ; Começa a ler a partir do setor 2
    mov dh, 0            ; Cabeça 0
    mov dl, 0x80         ; Drive 0x80 (primeiro disco rígido)

read_sector_loop:
    push cx              ; Salva contador CX na pilha
    mov ah, 0x02         ; Função da BIOS: ler setores do disco
    mov al, 1            ; Número de setores a ler por vez (1)
    int 0x13             ; Chama BIOS para ler setor
    jc disk_error        ; Se carry flag setada, ocorreu erro, vai para tratamento

    add di, 512          ; Incrementa DI para próximo buffer (+512 bytes por setor)
    inc cl               ; Próximo setor no disco
    pop cx               ; Restaura CX
    loop read_sector_loop ; Decrementa CX, se não zero, repete o loop

    jmp enter_pm         ; Kernel carregado, entra em modo protegido

disk_error:
    mov si, disk_error_msg ; Aponta SI para mensagem de erro

print_disk_error:
    lodsb                 ; Carrega byte da mensagem em AL, incrementa SI
    or al, al             ; Testa fim da string
    jz hang               ; Se zero, pula para loop infinito
    mov ah, 0x0E          ; Função teletipo BIOS para imprimir caractere
    int 0x10              ; Chama BIOS para imprimir caractere
    jmp print_disk_error  ; Repete para imprimir mensagem toda

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

