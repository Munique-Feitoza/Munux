/*
 * Munux Kernel - libk: conversão numérica.
 *
 * Toda a aritmética é de 32 bits para não depender dos helpers de 64 bits
 * do compilador (__udivdi3 etc.), que não são linkados no kernel freestanding.
 */

#include "libk.h"

char* utoa(uint32_t value, char* buf, int base) {
    if (base < 2 || base > 16) {
        buf[0] = '\0';
        return buf;
    }

    static const char digits[] = "0123456789abcdef";
    char tmp[33]; // pior caso: 32 dígitos binários
    int i = 0;

    if (value == 0) {
        tmp[i++] = '0';
    } else {
        while (value > 0) {
            tmp[i++] = digits[value % (uint32_t)base];
            value /= (uint32_t)base;
        }
    }

    // tmp está em ordem reversa; copia de trás para frente.
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
    return buf;
}

char* itoa(int32_t value, char* buf, int base) {
    if (base == 10 && value < 0) {
        buf[0] = '-';
        // Magnitude via unsigned (complemento de dois), sem overflow em INT_MIN.
        uint32_t mag = ~(uint32_t)value + 1u;
        utoa(mag, buf + 1, 10);
        return buf;
    }
    return utoa((uint32_t)value, buf, base);
}

int atoi(const char* s) {
    while (isspace((int)(unsigned char)*s)) {
        s++;
    }

    int sign = 1;
    if (*s == '+' || *s == '-') {
        if (*s == '-') {
            sign = -1;
        }
        s++;
    }

    int result = 0;
    while (isdigit((int)(unsigned char)*s)) {
        result = result * 10 + (*s - '0');
        s++;
    }
    return sign * result;
}
