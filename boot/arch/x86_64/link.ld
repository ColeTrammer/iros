/* Sets first thing to be executed to be start */
ENTRY(start)

SECTIONS
{
    /* Sets Boot loader to begin at 1M (for proper loading) */
    . = 1M;

    /* Put multiboot .first, then .text */
    .text ALIGN (0x1000) : AT(ADDR(.text))
    {
        *(.multiboot)
        *(.text)
    }

    /* Put read only data */
    .rodata ALIGN (0x1000) : AT(ADDR(.rodata))
    {
        *(.rodata*)
    }

    /* Put data */
    .data ALIGN (0x1000) : AT(ADDR(.data))
    {
        *(.data)
    }

    /* Put uninitialized data */
    .bss ALIGN (0x1000) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss)
    }
}