; Multiboot header para compatibilidade com GRUB
; Especificação Multiboot v1

section .multiboot
align 4

; Constantes Multiboot
MULTIBOOT_MAGIC        equ 0x1BADB002  ; Magic number que identifica o header
MULTIBOOT_ALIGN        equ 1 << 0      ; Align módulos carregados em páginas de 4KB
MULTIBOOT_MEMINFO      equ 1 << 1      ; Fornecer informação de memória
MULTIBOOT_FLAGS        equ MULTIBOOT_ALIGN | MULTIBOOT_MEMINFO
MULTIBOOT_CHECKSUM     equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

; Header Multiboot
dd MULTIBOOT_MAGIC
dd MULTIBOOT_FLAGS
dd MULTIBOOT_CHECKSUM

section .text
global _start
extern kernel_main

_start:
    ; Configurar stack
    mov esp, stack_top
    
    ; Passa as infos do Multiboot para o kernel (convenção cdecl, args da
    ; direita p/ esquerda):
    ;   EAX = magic (0x2BADB002 quando viemos de um loader Multiboot)
    ;   EBX = ponteiro para a struct multiboot_info
    push ebx            ; arg2: endereço da multiboot info
    push eax            ; arg1: magic
    call kernel_main
    
    ; Se o kernel retornar, entra em loop infinito
.hang:
    cli
    hlt
    jmp .hang

section .bss
align 16
stack_bottom:
    resb 16384  ; 16 KB de stack
stack_top:
