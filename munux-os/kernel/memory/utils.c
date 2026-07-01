/*
 * Munux Kernel - Memory Utilities
 * 
 * Funções auxiliares de manipulação de memória.
 */

#include "memory.h"

// Preenche memória com valor
void* memset(void* dest, int val, size_t len) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < len; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

// Copia memória
void* memcpy(void* dest, const void* src, size_t len) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Compara memória
int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* a = (const uint8_t*)s1;
    const uint8_t* b = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
    }
    return 0;
}
