/*
 * Munux Kernel - PS/2 Mouse Driver
 * 
 * Driver básico para mouse PS/2.
 */

#ifndef MOUSE_H
#define MOUSE_H

#include "../kernel.h"

// Estado do mouse
typedef struct {
    int32_t x;
    int32_t y;
    uint8_t buttons;  // Bit 0: esquerdo, Bit 1: direito, Bit 2: meio
} mouse_state_t;

// Funções
void mouse_init(void);
mouse_state_t mouse_get_state(void);
void mouse_set_callback(void (*callback)(int32_t dx, int32_t dy, uint8_t buttons));

#endif // MOUSE_H
