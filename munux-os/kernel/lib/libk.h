/*
 * Munux Kernel - libk (biblioteca C freestanding do kernel)
 *
 * Funções de string, conversão numérica e classificação de caracteres.
 * Sem dependência de libc — tudo implementado do zero para o ambiente
 * bare-metal. `strlen` continua declarada em kernel.h.
 */

#ifndef LIBK_H
#define LIBK_H

#include "../kernel.h"
#include <stdarg.h>

// --- string ---
int   strcmp(const char* a, const char* b);
int   strncmp(const char* a, const char* b, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);

// --- conversão numérica ---
// utoa/itoa escrevem em `buf` (o chamador garante espaço: base 10 cabe em
// 12 bytes com sinal e NUL; base 2 precisa de até 33).
char* utoa(uint32_t value, char* buf, int base); // base 2..16
char* itoa(int32_t value, char* buf, int base);   // base 10 trata o sinal
int   atoi(const char* s);

// --- formatação (printf) ---
// Bounds-checked: nunca escrevem além de `size` e sempre terminam em NUL.
// Retornam quantos caracteres seriam escritos (como snprintf).
int ksnprintf(char* buf, size_t size, const char* fmt, ...)
    __attribute__((format(printf, 3, 4)));
int kvsnprintf(char* buf, size_t size, const char* fmt, va_list args);

// --- classificação de caracteres (ctype), inline para custo zero ---
static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static inline int islower(int c) { return c >= 'a' && c <= 'z'; }
static inline int isalpha(int c) { return isupper(c) || islower(c); }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
static inline int isspace(int c) {
    return c == ' ' || c == '\t' || c == '\n' ||
           c == '\r' || c == '\v' || c == '\f';
}
static inline int toupper(int c) { return islower(c) ? c - 32 : c; }
static inline int tolower(int c) { return isupper(c) ? c + 32 : c; }

#endif // LIBK_H
