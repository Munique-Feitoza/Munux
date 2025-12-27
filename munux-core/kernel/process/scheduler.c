/*
 * Munux Kernel - Scheduler Implementation
 * 
 * Implementa scheduler round-robin preemptivo com prioridades.
 */

#include "process.h"
#include "../drivers/timer.h"
#include "../interrupts/idt.h"

// Filas de processos por prioridade
static process_t* ready_queue[4] = {0}; // Uma fila por prioridade

// Processo atual
extern process_t* current_process;
extern process_t* idle_process;

// Callback do timer para preempção
static void scheduler_tick(registers_t regs) {
    (void)regs;
    
    if (!current_process) return;
    
    // Decrementa quantum
    if (current_process->quantum > 0) {
        current_process->quantum--;
        current_process->total_time++;
    }
    
    // Se quantum acabou, agenda outro processo
    if (current_process->quantum == 0) {
        schedule();
    }
}

// Inicializa scheduler
void scheduler_init(void) {
    // Registra callback do timer
    register_interrupt_handler(32, scheduler_tick);
    
    // Zera filas
    for (int i = 0; i < 4; i++) {
        ready_queue[i] = 0;
    }
}

// Adiciona processo à fila de prontos
void scheduler_add_process(process_t* process) {
    if (!process) return;
    
    process->state = PROCESS_READY;
    process->quantum = DEFAULT_QUANTUM;
    
    uint32_t prio = process->priority;
    if (prio > 3) prio = 3;
    
    // Adiciona ao fim da fila
    if (!ready_queue[prio]) {
        ready_queue[prio] = process;
        process->next = process; // Fila circular
    } else {
        process_t* last = ready_queue[prio];
        while (last->next != ready_queue[prio]) {
            last = last->next;
        }
        last->next = process;
        process->next = ready_queue[prio];
    }
}

// Remove processo da fila
void scheduler_remove_process(process_t* process) {
    if (!process) return;
    
    uint32_t prio = process->priority;
    if (prio > 3) prio = 3;
    
    if (!ready_queue[prio]) return;
    
    // Se é único na fila
    if (ready_queue[prio]->next == ready_queue[prio]) {
        ready_queue[prio] = 0;
        return;
    }
    
    // Procura e remove
    process_t* p = ready_queue[prio];
    do {
        if (p->next == process) {
            p->next = process->next;
            if (ready_queue[prio] == process) {
                ready_queue[prio] = process->next;
            }
            return;
        }
        p = p->next;
    } while (p != ready_queue[prio]);
}

// Seleciona próximo processo
static process_t* select_next_process(void) {
    // Procura da maior para menor prioridade
    for (int prio = 3; prio >= 0; prio--) {
        if (ready_queue[prio]) {
            process_t* next = ready_queue[prio];
            ready_queue[prio] = next->next;
            return next;
        }
    }
    
    // Nenhum processo, retorna idle
    return idle_process;
}

// Agenda próximo processo
void schedule(void) {
    if (!current_process) return;
    
    // Salva processo atual de volta na fila se ainda está rodando
    if (current_process->state == PROCESS_RUNNING) {
        scheduler_add_process(current_process);
    }
    
    // Seleciona próximo
    process_t* next = select_next_process();
    if (!next) next = idle_process;
    
    // Se for o mesmo, só reseta quantum
    if (next == current_process) {
        current_process->quantum = DEFAULT_QUANTUM;
        return;
    }
    
    // Context switch
    process_t* old = current_process;
    current_process = next;
    next->state = PROCESS_RUNNING;
    next->quantum = DEFAULT_QUANTUM;
    
    // Troca contexto
    switch_to_process(&old->context, &next->context);
}

// Yield: processo cede CPU voluntariamente
void yield(void) {
    current_process->quantum = 0;
    schedule();
}
