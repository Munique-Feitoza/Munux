ENTRY(_start)

SECTIONS
{
  . = 0x00100000; /* Carregar kernel em 1MB (padrão multiboot) */

  /* Header multiboot DEVE vir primeiro */
  .multiboot ALIGN(4K) : {
    *(.multiboot)
  }

  .text ALIGN(4K) : {
    *(.text)
  }

  .rodata ALIGN(4K) : {
    *(.rodata)
  }

  .data ALIGN(4K) : {
    *(.data)
  }

  .bss ALIGN(4K) : {
    *(.bss)
    *(COMMON)
  }
}

