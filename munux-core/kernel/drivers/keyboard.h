/*
 * Munux Kernel - Keyboard Driver
 * 
 * Driver completo de teclado PS/2 com:
 * - Mapeamento de scancodes
 * - Buffer circular
 * - Suporte a Shift, Ctrl, Alt, Caps Lock
 * - Layout ABNT2 (BR)
 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "../kernel.h"

// Tamanho do buffer de teclado
#define KEYBOARD_BUFFER_SIZE 256

// Códigos especiais
#define KEY_NULL        0
#define KEY_ESC         27
#define KEY_BACKSPACE   '\b'
#define KEY_TAB         '\t'
#define KEY_ENTER       '\n'
#define KEY_RETURN      '\r'
#define KEY_CTRL        0x80
#define KEY_SHIFT       0x81
#define KEY_ALT         0x82
#define KEY_CAPS_LOCK   0x83
#define KEY_F1          0x84
#define KEY_F2          0x85
#define KEY_F3          0x86
#define KEY_F4          0x87
#define KEY_F5          0x88
#define KEY_F6          0x89
#define KEY_F7          0x8A
#define KEY_F8          0x8B
#define KEY_F9          0x8C
#define KEY_F10         0x8D
#define KEY_F11         0x8E
#define KEY_F12         0x8F
#define KEY_UP          0x90
#define KEY_DOWN        0x91
#define KEY_LEFT        0x92
#define KEY_RIGHT       0x93
#define KEY_HOME        0x94
#define KEY_END         0x95
#define KEY_PAGE_UP     0x96
#define KEY_PAGE_DOWN   0x97
#define KEY_INSERT      0x98
#define KEY_DELETE      0x99

// Estado das teclas modificadoras
typedef struct {
    uint8_t shift   : 1;
    uint8_t ctrl    : 1;
    uint8_t alt     : 1;
    uint8_t caps    : 1;
    uint8_t num     : 1;
    uint8_t scroll  : 1;
} keyboard_state_t;

// Funções principais
void keyboard_init(void);
char keyboard_getchar(void);
int keyboard_available(void);
void keyboard_flush(void);
keyboard_state_t keyboard_get_state(void);

#endif // KEYBOARD_H
