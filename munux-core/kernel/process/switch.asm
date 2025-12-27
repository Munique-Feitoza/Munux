; Munux Kernel - Context Switch
; 
; Realiza troca de contexto entre processos

[BITS 32]

; void switch_to_process(cpu_context_t* old_context, cpu_context_t* new_context);
global switch_to_process
switch_to_process:
    mov eax, [esp + 4]  ; old_context
    mov edx, [esp + 8]  ; new_context
    
    ; Salva contexto do processo antigo
    mov [eax + 0],  dword 0      ; eax (será salvo depois)
    mov [eax + 4],  ebx
    mov [eax + 8],  ecx
    mov [eax + 12], dword 0      ; edx (será salvo depois)
    mov [eax + 16], esi
    mov [eax + 20], edi
    mov [eax + 24], ebp
    mov [eax + 28], esp
    
    ; Salva EIP (endereço de retorno)
    mov ebx, [esp]
    mov [eax + 32], ebx
    
    ; Salva EFLAGS
    pushfd
    pop ebx
    mov [eax + 36], ebx
    
    ; Salva CR3 (page directory)
    mov ebx, cr3
    mov [eax + 40], ebx
    
    ; Restaura contexto do novo processo
    mov ebx, [edx + 4]   ; ebx
    mov ecx, [edx + 8]   ; ecx
    mov esi, [edx + 16]  ; esi
    mov edi, [edx + 20]  ; edi
    mov ebp, [edx + 24]  ; ebp
    mov esp, [edx + 28]  ; esp
    
    ; Restaura CR3 se diferente
    mov eax, [edx + 40]  ; CR3 do novo processo
    mov ebx, cr3
    cmp eax, ebx
    je .no_cr3_change
    mov cr3, eax
.no_cr3_change:
    
    ; Restaura EFLAGS
    push dword [edx + 36]
    popfd
    
    ; Salta para EIP do novo processo
    mov eax, [edx + 0]   ; eax
    jmp [edx + 32]       ; eip
