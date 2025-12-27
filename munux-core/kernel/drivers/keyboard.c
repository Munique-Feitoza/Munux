/*
 * Munux Kernel - Keyboard Driver Implementation
 */

#include "keyboard.h"
#include "../interrupts/idt.h"
#include "../interrupts/io.h"
#include "../kernel.h"

// Buffer circular para teclado
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile uint32_t buffer_read_pos = 0;
static volatile uint32_t buffer_write_pos = 0;

// Estado do teclado
static keyboard_state_t kbd_state = {0};

// Layout ABNT2 - scancodes para ASCII (sem shift)
static const char scancode_to_ascii[] = {
    0,    KEY_ESC, '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8',  '9',  '0',  '-',  '=',  KEY_BACKSPACE, KEY_TAB, // 0x08-0x0F
    'q',  'w',  'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17
    'o',  'p',  '[',  ']',  KEY_ENTER, KEY_CTRL, 'a',  's',   // 0x18-0x1F
    'd',  'f',  'g',  'h',  'j',  'k',  'l',  ';',   // 0x20-0x27
    '\'', '`',  KEY_SHIFT, '\\', 'z',  'x',  'c',  'v',   // 0x28-0x2F
    'b',  'n',  'm',  ',',  '.',  '/',  KEY_SHIFT, '*',   // 0x30-0x37
    KEY_ALT, ' ',  KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,  // 0x38-0x3F
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0,    0,    '7',   // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    KEY_F11,  // 0x50-0x57
    KEY_F12                                             // 0x58
};

// Layout ABNT2 - scancodes para ASCII (com shift)
static const char scancode_to_ascii_shift[] = {
    0,    KEY_ESC, '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*',  '(',  ')',  '_',  '+',  KEY_BACKSPACE, KEY_TAB, // 0x08-0x0F
    'Q',  'W',  'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P',  '{',  '}',  KEY_ENTER, KEY_CTRL, 'A',  'S',   // 0x18-0x1F
    'D',  'F',  'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~',  KEY_SHIFT, '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N',  'M',  '<',  '>',  '?',  KEY_SHIFT, '*',   // 0x30-0x37
    KEY_ALT, ' ',  KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5,  // 0x38-0x3F
    KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, 0,    0,    '7',   // 0x40-0x47
    '8',  '9',  '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3',  '0',  '.',  0,    0,    0,    KEY_F11,  // 0x50-0x57
    KEY_F12                                             // 0x58
};

// Adiciona caractere ao buffer
static void keyboard_buffer_put(char c) {
    uint32_t next_pos = (buffer_write_pos + 1) % KEYBOARD_BUFFER_SIZE;
    
    // Só adiciona se buffer não estiver cheio
    if (next_pos != buffer_read_pos) {
        keyboard_buffer[buffer_write_pos] = c;
        buffer_write_pos = next_pos;
    }
}

// Handler de interrupção do teclado
static void keyboard_handler(registers_t regs) {
    (void)regs; // Não usado
    
    uint8_t scancode = inb(0x60);
    
    // Verifica se é release (bit 7 setado)
    uint8_t pressed = !(scancode & 0x80);
    scancode &= 0x7F;
    
    // Trata teclas especiais
    if (scancode == 0x2A || scancode == 0x36) { // Shift esquerdo/direito
        kbd_state.shift = pressed;
        return;
    }
    if (scancode == 0x1D) { // Ctrl
        kbd_state.ctrl = pressed;
        return;
    }
    if (scancode == 0x38) { // Alt
        kbd_state.alt = pressed;
        return;
    }
    if (scancode == 0x3A && pressed) { // Caps Lock
        kbd_state.caps = !kbd_state.caps;
        return;
    }
    
    // Só processa se for press (não release)
    if (!pressed) {
        return;
    }
    
    // Converte scancode para ASCII
    char ascii = 0;
    
    if (scancode < sizeof(scancode_to_ascii)) {
        if (kbd_state.shift) {
            ascii = scancode_to_ascii_shift[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
        
        // Aplica Caps Lock para letras
        if (kbd_state.caps && ascii >= 'a' && ascii <= 'z') {
            ascii -= 32; // Converte para maiúscula
        } else if (kbd_state.caps && ascii >= 'A' && ascii <= 'Z' && kbd_state.shift) {
            ascii += 32; // Converte para minúscula se Shift + Caps
        }
        
        if (ascii != 0) {
            keyboard_buffer_put(ascii);
        }
    }
}

// Inicializa o driver de teclado
void keyboard_init(void) {
    // Registra handler para IRQ1 (teclado)
    register_interrupt_handler(33, keyboard_handler);
    
    // Limpa buffer
    buffer_read_pos = 0;
    buffer_write_pos = 0;
    
    // Reseta estado
    kbd_state.shift = 0;
    kbd_state.ctrl = 0;
    kbd_state.alt = 0;
    kbd_state.caps = 0;
    kbd_state.num = 0;
    kbd_state.scroll = 0;
}

// Lê caractere do buffer (bloqueante)
char keyboard_getchar(void) {
    while (buffer_read_pos == buffer_write_pos) {
        __asm__ volatile("hlt"); // Espera interrupção
    }
    
    char c = keyboard_buffer[buffer_read_pos];
    buffer_read_pos = (buffer_read_pos + 1) % KEYBOARD_BUFFER_SIZE;
    return c;
}

// Verifica se há caracteres disponíveis
int keyboard_available(void) {
    return buffer_read_pos != buffer_write_pos;
}

// Limpa o buffer
void keyboard_flush(void) {
    buffer_read_pos = 0;
    buffer_write_pos = 0;
}

// Retorna estado atual do teclado
keyboard_state_t keyboard_get_state(void) {
    return kbd_state;
}
