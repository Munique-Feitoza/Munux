/*
 * Munux Kernel Header
 * 
 * Definições de tipos, constantes e protótipos de funções
 * para o kernel do Munux.
 */

#ifndef KERNEL_H
#define KERNEL_H

// Definições de tipos básicos
typedef unsigned char   uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int    uint32_t;
typedef unsigned long long uint64_t;
typedef signed char     int8_t;
typedef signed short    int16_t;
typedef signed int      int32_t;
typedef signed long long int64_t;
typedef unsigned int    size_t;

// Definições para cores no modo texto VGA
#define COLOR_BLACK         0
#define COLOR_BLUE          1
#define COLOR_GREEN         2
#define COLOR_CYAN          3
#define COLOR_RED           4
#define COLOR_MAGENTA       5
#define COLOR_BROWN         6
#define COLOR_LIGHT_GREY    7
#define COLOR_DARK_GREY     8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_LIGHT_BROWN   14
#define COLOR_WHITE         15

// Constantes do terminal VGA
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

// Protótipos de funções do terminal
void terminal_clear(void);
void terminal_setcolor(uint8_t color);
void terminal_putchar(char c);
void terminal_writestring(const char* data);

// Protótipos de funções utilitárias
size_t strlen(const char* str);

// Função principal do kernel
void kernel_main(void);

// Caminho de pânico — chamado tanto pelo lado C quanto pelo
// `#[panic_handler]` da camada Rust.
__attribute__((noreturn))
void kernel_panic(const char* msg);

#endif // KERNEL_H
