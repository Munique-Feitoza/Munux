/* Linker script — Munux z/OS (s390x).
 * Carga em 0x10000 (endereço onde o firmware s390-ccw do QEMU coloca o kernel
 * passado por -kernel), entry `start`. */
ENTRY(start)

SECTIONS
{
    . = 0x10000;

    .text : {
        *(.init)
        *(.text .text.*)
    }

    .rodata : { *(.rodata .rodata.*) }
    .data   : { *(.data .data.*) }

    .bss : {
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        __bss_end = .;
    }

    . = ALIGN(16);
    . += 0x4000;          /* 16 KiB de stack */
    stackptr = .;

    /DISCARD/ : { *(.eh_frame) *(.comment) }
}
