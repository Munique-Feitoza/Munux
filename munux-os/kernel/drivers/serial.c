/*
 * Munux Kernel - Serial Port Driver Implementation
 */

#include "serial.h"
#include "../interrupts/io.h"

// Verifica se pode transmitir
static int serial_is_transmit_empty(uint16_t port) {
    return inb(port + 5) & 0x20;
}

// Inicializa porta serial
int serial_init(uint16_t port) {
    outb(port + 1, 0x00);    // Desabilita interrupções
    outb(port + 3, 0x80);    // Habilita DLAB (set baud rate divisor)
    outb(port + 0, 0x03);    // Divisor baixo (38400 baud)
    outb(port + 1, 0x00);    // Divisor alto
    outb(port + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(port + 2, 0xC7);    // Habilita FIFO, clear com threshold de 14 bytes
    outb(port + 4, 0x0B);    // IRQs habilitadas, RTS/DSR set
    
    // Teste de loopback
    outb(port + 4, 0x1E);    // Modo loopback
    outb(port + 0, 0xAE);    // Envia byte de teste
    
    if (inb(port + 0) != 0xAE) {
        return 1; // Falha
    }
    
    // Normal mode
    outb(port + 4, 0x0F);
    return 0;
}

// Escreve caractere
void serial_putchar(uint16_t port, char c) {
    while (!serial_is_transmit_empty(port));
    outb(port, c);
}

// Escreve string
void serial_writestring(uint16_t port, const char* str) {
    for (size_t i = 0; str[i]; i++) {
        serial_putchar(port, str[i]);
    }
}

// Verifica se há dados recebidos
int serial_received(uint16_t port) {
    return inb(port + 5) & 1;
}

// Lê caractere
char serial_getchar(uint16_t port) {
    while (!serial_received(port));
    return inb(port);
}
