/*
 * Munux Kernel - Serial Port Driver
 * 
 * Driver para porta serial (COM1/COM2) usado principalmente
 * para debug e comunicação com console externo.
 */

#ifndef SERIAL_H
#define SERIAL_H

#include "../kernel.h"

// Portas seriais
#define COM1 0x3F8
#define COM2 0x2F8
#define COM3 0x3E8
#define COM4 0x2E8

// Inicializa porta serial
int serial_init(uint16_t port);

// Escreve caractere
void serial_putchar(uint16_t port, char c);

// Escreve string
void serial_writestring(uint16_t port, const char* str);

// Lê caractere (bloqueante)
char serial_getchar(uint16_t port);

// Verifica se há dados disponíveis
int serial_received(uint16_t port);

#endif // SERIAL_H
