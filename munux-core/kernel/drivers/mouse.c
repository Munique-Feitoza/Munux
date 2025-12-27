/*
 * Munux Kernel - PS/2 Mouse Driver Implementation
 */

#include "mouse.h"
#include "../interrupts/idt.h"
#include "../interrupts/io.h"

static mouse_state_t mouse_state = {0, 0, 0};
static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];
static void (*mouse_callback)(int32_t, int32_t, uint8_t) = 0;

// Espera até poder escrever
static void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (!(inb(0x64) & 2)) return;
    }
}

// Espera até poder ler
static void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (inb(0x64) & 1) return;
    }
}

// Escreve para o mouse
static void mouse_write(uint8_t data) {
    mouse_wait_write();
    outb(0x64, 0xD4);
    mouse_wait_write();
    outb(0x60, data);
}

// Lê do mouse
static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

// Handler de interrupção
static void mouse_handler(registers_t regs) {
    (void)regs;
    
    mouse_byte[mouse_cycle++] = inb(0x60);
    
    if (mouse_cycle == 3) {
        mouse_cycle = 0;
        
        // Parseia pacote
        uint8_t status = mouse_byte[0];
        int32_t dx = mouse_byte[1];
        int32_t dy = mouse_byte[2];
        
        // Extende sinal se negativo
        if (dx && (status & (1 << 4))) dx -= 256;
        if (dy && (status & (1 << 5))) dy -= 256;
        
        // Inverte Y (PS/2 tem Y invertido)
        dy = -dy;
        
        // Atualiza estado
        mouse_state.x += dx;
        mouse_state.y += dy;
        mouse_state.buttons = status & 0x07;
        
        // Chama callback se registrado
        if (mouse_callback) {
            mouse_callback(dx, dy, mouse_state.buttons);
        }
    }
}

// Inicializa mouse
void mouse_init(void) {
    // Registra handler IRQ12
    register_interrupt_handler(44, mouse_handler);
    
    // Habilita dispositivo auxiliar
    mouse_wait_write();
    outb(0x64, 0xA8);
    
    // Habilita interrupções
    mouse_wait_write();
    outb(0x64, 0x20);
    mouse_wait_read();
    uint8_t status = inb(0x60) | 2;
    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);
    
    // Usa configurações padrão
    mouse_write(0xF6);
    mouse_read(); // ACK
    
    // Habilita streaming de dados
    mouse_write(0xF4);
    mouse_read(); // ACK
    
    // Reseta estado
    mouse_state.x = 0;
    mouse_state.y = 0;
    mouse_state.buttons = 0;
}

// Retorna estado atual
mouse_state_t mouse_get_state(void) {
    return mouse_state;
}

// Registra callback
void mouse_set_callback(void (*callback)(int32_t dx, int32_t dy, uint8_t buttons)) {
    mouse_callback = callback;
}
