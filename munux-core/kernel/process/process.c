/*
 * Munux Kernel - Process Management Implementation
 */

#include "process.h"
#include "../interrupts/idt.h"

// Processo atual e idle
static process_t* current_process = 0;
static process_t* idle_process = 0;

// Lista de processos
static process_t* process_list = 0;

// Próximo PID disponível
static uint32_t next_pid = 1;

// Quantum padrão (em ticks)
#define DEFAULT_QUANTUM 10

// Processo idle (executa quando não há nada para fazer)
static void idle_task(void) {
    while (1) {
        __asm__ volatile("hlt");
    }
}

// Inicializa sistema de processos
void process_init(void) {
    // Cria processo idle
    idle_process = process_create("idle", idle_task, PRIORITY_LOW);
    current_process = idle_process;
}

// Cria novo processo
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority) {
    process_t* process = (process_t*)kmalloc(sizeof(process_t));
    if (!process) return 0;
    
    // Inicializa PCB
    process->pid = next_pid++;
    
    // Copia nome
    int i;
    for (i = 0; i < 31 && name[i]; i++) {
        process->name[i] = name[i];
    }
    process->name[i] = '\0';
    
    process->state = PROCESS_READY;
    process->priority = priority;
    process->quantum = DEFAULT_QUANTUM;
    process->total_time = 0;
    process->parent = current_process;
    process->next = 0;
    
    // Aloca stacks
    process->kernel_stack = (uint32_t)kmalloc(8192) + 8192; // 8KB stack
    process->user_stack = (uint32_t)kmalloc(8192) + 8192;
    
    // Cria page directory (clone do kernel por enquanto)
    process->page_dir = vmm_get_current_directory();
    
    // Inicializa contexto
    memset(&process->context, 0, sizeof(cpu_context_t));
    process->context.eip = (uint32_t)entry_point;
    process->context.esp = process->user_stack;
    process->context.ebp = process->user_stack;
    process->context.eflags = 0x202; // IF habilitado
    process->context.cr3 = (uint32_t)process->page_dir;
    
    // Adiciona à lista de processos
    if (!process_list) {
        process_list = process;
    } else {
        process_t* p = process_list;
        while (p->next) p = p->next;
        p->next = process;
    }
    
    return process;
}

// Termina processo
void process_terminate(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_TERMINATED;
    
    // Remove da lista
    if (process_list == process) {
        process_list = process->next;
    } else {
        process_t* p = process_list;
        while (p && p->next != process) {
            p = p->next;
        }
        if (p) {
            p->next = process->next;
        }
    }
    
    // Libera memória
    kfree((void*)(process->kernel_stack - 8192));
    kfree((void*)(process->user_stack - 8192));
    kfree(process);
    
    // Se for processo atual, agenda outro
    if (current_process == process) {
        schedule();
    }
}

// Retorna processo atual
process_t* process_get_current(void) {
    return current_process;
}

// Encontra processo por PID
process_t* process_find_by_pid(uint32_t pid) {
    process_t* p = process_list;
    while (p) {
        if (p->pid == pid) return p;
        p = p->next;
    }
    return 0;
}
