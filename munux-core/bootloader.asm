; Bootloader básico em Assembly - Munux Core
; Carregado pela BIOS na memória em 0x7C00 (endereço padrão para bootloader)

[org 0x7C00]             ; Define o ponto inicial da execução do bootloader na memória

mov ah, 0x0E             ; Função da BIOS para imprimir caractere na tela (modo texto)
mov si, msg              ; Aponta o registrador SI para o início da mensagem

print_loop:
    lodsb                ; Carrega o próximo byte da mensagem para AL e incrementa SI
    cmp al, 0            ; Compara o byte atual com 0 (final da string)
    je done              ; Se for zero, fim da mensagem, pula para 'done'
    int 0x10             ; Chama a interrupção da BIOS para imprimir caractere em AL
    jmp print_loop       ; Loop para continuar imprimindo o próximo caractere

done:
    jmp $                ; Loop infinito para travar o sistema aqui (para teste)

; A mensagem que será mostrada na tela
msg db "Iniciando Munux...", 0

; Preenche o restante do setor com zeros até o byte 510
times 510-($-$$) db 0

; Assinatura mágica do bootloader (últimos 2 bytes do setor)
dw 0xAA55

