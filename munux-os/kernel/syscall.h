/*
 * Munux Kernel - Interface de Syscalls (int 0x80)
 *
 * Convenção: eax = número da syscall, ebx/ecx/edx = argumentos,
 * valor de retorno em eax. O dispatcher roda em Ring 0; a validação de
 * ponteiros de user mode virá com a separação de privilégios (v0.5).
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "interrupts/idt.h"

// Números das syscalls.
#define SYS_WRITE      1   // write(fd, buf, len)   -> bytes escritos
#define SYS_GET_TICKS  2   // get_ticks()           -> ticks do timer
#define SYS_OPEN       3   // open(path, len)       -> fd (>=3) ou -1
#define SYS_READ       4   // read(fd, buf, len)    -> bytes, 0 EOF, -1 erro
#define SYS_CLOSE      5   // close(fd)             -> 0 ou -1

// Dispatcher chamado pelo stub de int 0x80.
void syscall_handler(registers_t* regs);

#endif // SYSCALL_H
