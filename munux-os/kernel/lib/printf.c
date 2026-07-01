/*
 * Munux Kernel - libk: formatação estilo printf (bounds-checked).
 *
 * Suporta: %d %i %u %x %X %s %c %p %%, com flag de zero-pad e largura mínima
 * (ex.: %08x). Sem precisão nem modificadores de tamanho — inteiros de 32 bits.
 * `kvsnprintf` nunca escreve além de `size` e sempre termina em NUL.
 */

#include "libk.h"

static void putc_buf(char* buf, size_t size, size_t* total, char c) {
    if (*total + 1 < size) { // reserva 1 byte para o NUL final
        buf[*total] = c;
    }
    (*total)++;
}

static void puts_buf(char* buf, size_t size, size_t* total, const char* s) {
    while (*s) {
        putc_buf(buf, size, total, *s++);
    }
}

// Emite `digits` (número já convertido) com sinal opcional e padding.
static void emit_num(char* buf, size_t size, size_t* total,
                     const char* digits, char sign, int width, int zero) {
    int len = 0;
    while (digits[len]) {
        len++;
    }
    int field = len + (sign ? 1 : 0);

    if (zero) {
        if (sign) {
            putc_buf(buf, size, total, sign);
        }
        for (int i = field; i < width; i++) {
            putc_buf(buf, size, total, '0');
        }
    } else {
        for (int i = field; i < width; i++) {
            putc_buf(buf, size, total, ' ');
        }
        if (sign) {
            putc_buf(buf, size, total, sign);
        }
    }
    puts_buf(buf, size, total, digits);
}

int kvsnprintf(char* buf, size_t size, const char* fmt, va_list args) {
    size_t total = 0;
    char numbuf[33];

    for (const char* p = fmt; *p; p++) {
        if (*p != '%') {
            putc_buf(buf, size, &total, *p);
            continue;
        }

        p++;
        int zero = 0;
        if (*p == '0') {
            zero = 1;
            p++;
        }
        int width = 0;
        while (isdigit((int)(unsigned char)*p)) {
            width = width * 10 + (*p - '0');
            p++;
        }

        switch (*p) {
        case 'c':
            putc_buf(buf, size, &total, (char)va_arg(args, int));
            break;
        case 's': {
            const char* s = va_arg(args, const char*);
            puts_buf(buf, size, &total, s ? s : "(null)");
            break;
        }
        case 'd':
        case 'i': {
            int v = va_arg(args, int);
            char sign = 0;
            uint32_t mag;
            if (v < 0) {
                sign = '-';
                mag = ~(uint32_t)v + 1u;
            } else {
                mag = (uint32_t)v;
            }
            utoa(mag, numbuf, 10);
            emit_num(buf, size, &total, numbuf, sign, width, zero);
            break;
        }
        case 'u':
            utoa(va_arg(args, uint32_t), numbuf, 10);
            emit_num(buf, size, &total, numbuf, 0, width, zero);
            break;
        case 'x':
            utoa(va_arg(args, uint32_t), numbuf, 16);
            emit_num(buf, size, &total, numbuf, 0, width, zero);
            break;
        case 'X':
            utoa(va_arg(args, uint32_t), numbuf, 16);
            for (char* q = numbuf; *q; q++) {
                *q = (char)toupper((int)(unsigned char)*q);
            }
            emit_num(buf, size, &total, numbuf, 0, width, zero);
            break;
        case 'p':
            puts_buf(buf, size, &total, "0x");
            utoa((uint32_t)(size_t)va_arg(args, void*), numbuf, 16);
            emit_num(buf, size, &total, numbuf, 0, width, zero);
            break;
        case '%':
            putc_buf(buf, size, &total, '%');
            break;
        case '\0':
            p--; // fmt terminou em '%'; recua para o laço encerrar
            break;
        default:
            putc_buf(buf, size, &total, '%');
            putc_buf(buf, size, &total, *p);
            break;
        }
    }

    if (size > 0) {
        buf[total < size ? total : size - 1] = '\0';
    }
    return (int)total;
}

int ksnprintf(char* buf, size_t size, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int n = kvsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}
