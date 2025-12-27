/*
 * Munux Kernel - Programmable Interval Timer (PIT)
 * 
 * Driver do timer do sistema, essencial para:
 * - Multitasking (scheduler)
 * - Contagem de tempo
 * - Delays precisos
 */

#ifndef TIMER_H
#define TIMER_H

#include "../kernel.h"

// Frequência do PIT (1.193182 MHz)
#define PIT_FREQUENCY 1193182

// Inicializa o timer com frequência em Hz
void timer_init(uint32_t frequency);

// Retorna número de ticks desde boot
uint32_t timer_get_ticks(void);

// Sleep por número de ticks
void timer_wait(uint32_t ticks);

#endif // TIMER_H
