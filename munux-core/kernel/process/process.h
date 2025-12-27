/*
 * Munux Kernel - Process Management
 * 
 * Sistema completo de gerenciamento de processos:
 * - Process Control Block (PCB)
 * - Scheduler round-robin preemptivo
 * - Context switching
 * - Criação/término de processos
 */

#ifndef PROCESS_H
#define PROCESS_H

#include "../kernel.h"
#include "../memory/memory.h"

// Estados de processo
typedef enum {
    PROCESS_READY,      // Pronto para executar
    PROCESS_RUNNING,    // Em execução
    PROCESS_BLOCKED,    // Bloqueado aguardando I/O
    PROCESS_TERMINATED  // Terminado
} process_state_t;

// Prioridades
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2,
    PRIORITY_REALTIME = 3
} process_priority_t;

// Constante de quantum padrão (50ms = 50 ticks)
#define DEFAULT_QUANTUM 50

// Contexto de CPU salvo
typedef struct {
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t eflags;
    uint32_t cr3;  // Page directory
} cpu_context_t;

// Process Control Block
typedef struct process {
    uint32_t pid;                    // Process ID
    char name[32];                   // Nome do processo
    process_state_t state;           // Estado atual
    process_priority_t priority;     // Prioridade
    cpu_context_t context;           // Contexto da CPU
    page_directory_t* page_dir;      // Page directory do processo
    uint32_t kernel_stack;           // Stack do kernel
    uint32_t user_stack;             // Stack do usuário
    uint32_t quantum;                // Quantum restante
    uint32_t total_time;             // Tempo total de CPU
    struct process* next;            // Próximo na fila
    struct process* parent;          // Processo pai
} process_t;

// Funções de gerenciamento de processos
void process_init(void);
process_t* process_create(const char* name, void (*entry_point)(void), process_priority_t priority);
void process_terminate(process_t* process);
process_t* process_get_current(void);
process_t* process_find_by_pid(uint32_t pid);

// Scheduler
void scheduler_init(void);
void scheduler_add_process(process_t* process);
void scheduler_remove_process(process_t* process);
void schedule(void);
void yield(void);

// Context switching
extern void switch_to_process(cpu_context_t* old_context, cpu_context_t* new_context);

#endif // PROCESS_H
