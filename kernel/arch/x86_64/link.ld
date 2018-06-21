/* Sets first thing to be executed to be start */
ENTRY(kernel_main)

SECTIONS
{
    /* Sets Boot loader to begin at 1M (for proper loading) */
    . = 0x10C000;

    /* Put multiboot .first, then .text */
    .text ALIGN (0x1000) : AT(ADDR(.text))
    {
        *(.multiboot)
        _text_start = .;
        *(.text)
        _text_end = .;
    }

    /* Put read only data */
    .rodata ALIGN (0x1000) : AT(ADDR(.rodata))
    {
        _rod_start = .;
        *(.rodata*)
        _rod_end = .;
    }

    /* Put data */
    .data ALIGN (0x1000) : AT(ADDR(.data))
    {
        _data_start = .;
        *(.data)
        _data_end = .;
    }

    /* Put uninitialized data */
    .bss ALIGN (0x1000) : AT(ADDR(.bss))
    {
        _bss_start = .;
        *(COMMON)
        *(.bss)
        _bss_end = .;
    }
}