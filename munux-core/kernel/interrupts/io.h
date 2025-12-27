/*
 * Munux Kernel - I/O Port Operations
 * 
 * Funções para comunicação com portas de I/O do hardware.
 */

#ifndef IO_H
#define IO_H

#include "../kernel.h"

// Escreve um byte em uma porta
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Lê um byte de uma porta
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Escreve uma word (2 bytes) em uma porta
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Lê uma word de uma porta
static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Escreve um dword (4 bytes) em uma porta
static inline void outl(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

// Lê um dword de uma porta
static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Pequeno delay usando I/O
static inline void io_wait(void) {
    outb(0x80, 0);
}

#endif // IO_H
