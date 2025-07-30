ENTRY(kernel_main)

SECTIONS
{
  . = 0x00080000; /* Carregar kernel em 512KB */

  .text : {
    *(.text)
  }

  .rodata : {
    *(.rodata)
  }

  .data : {
    *(.data)
  }

  .bss : {
    *(.bss)
    *(COMMON)
  }
}

