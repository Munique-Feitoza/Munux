/*
 * Munux Kernel - Dispatcher de Syscalls (int 0x80)
 */

#include "syscall.h"
#include "kernel.h"
#include "drivers/timer.h"
#include "drivers/serial.h"
#include "munux_rs.h"

// write(fd, buf, len): por enquanto só fd=1 (stdout), escrevendo na tela e na
// serial. Retorna o número de bytes escritos, ou (uint32_t)-1 em erro.
static uint32_t sys_write(uint32_t fd, const char* buf, uint32_t len) {
    if (fd != 1 || buf == 0) {
        return (uint32_t)-1;
    }
    for (uint32_t i = 0; i < len; i++) {
        terminal_putchar(buf[i]);
        serial_putchar(COM1, buf[i]);
    }
    return len;
}

void syscall_handler(registers_t* regs) {
    uint32_t ret;
    switch (regs->eax) {
    case SYS_WRITE:
        ret = sys_write(regs->ebx, (const char*)regs->ecx, regs->edx);
        break;
    case SYS_GET_TICKS:
        ret = timer_get_ticks();
        break;
    case SYS_OPEN:
        ret = (uint32_t)munux_rs_vfs_open((const uint8_t*)regs->ebx, regs->ecx);
        break;
    case SYS_READ:
        ret = (uint32_t)munux_rs_vfs_read((int32_t)regs->ebx, (uint8_t*)regs->ecx, regs->edx);
        break;
    case SYS_CLOSE:
        ret = (uint32_t)munux_rs_vfs_close((int32_t)regs->ebx);
        break;
    default:
        ret = (uint32_t)-1; // syscall desconhecida
        break;
    }
    // O valor de retorno volta pelo eax salvo (restaurado pelo popa do stub).
    regs->eax = ret;
}
