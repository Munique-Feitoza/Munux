/*
 * Munux Kernel - Programmable Interval Timer Implementation
 */

#include "timer.h"
#include "../interrupts/idt.h"
#include "../interrupts/io.h"
#include "../kernel.h"

// Contador de ticks
static volatile uint32_t timer_ticks = 0;

// Handler de interrupção do timer
static void timer_callback(registers_t regs) {
    (void)regs; // Não usado
    timer_ticks++;
}

// Inicializa o timer
void timer_init(uint32_t frequency) {
    // Registra callback para IRQ0 (timer)
    register_interrupt_handler(32, timer_callback);

    // Calcula divisor para frequência desejada
    uint32_t divisor = PIT_FREQUENCY / frequency;

    // Envia comando para o PIT
    outb(0x43, 0x36); // Command byte: channel 0, lobyte/hibyte, rate generator

    // Envia bytes do divisor
    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(0x40, low);
    outb(0x40, high);

    timer_ticks = 0;
}

// Retorna número de ticks
uint32_t timer_get_ticks(void) {
    return timer_ticks;
}

// Aguarda número específico de ticks
void timer_wait(uint32_t ticks) {
    uint32_t end_tick = timer_ticks + ticks;
    while (timer_ticks < end_tick) {
        __asm__ volatile("hlt"); // Halt até próxima interrupção
    }
}
