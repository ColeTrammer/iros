ENTRY(start)

SECTIONS
{
    . = 1M;

    .text ALIGN (0x1000) : AT(ADDR(.text))
    {
        *(.multiboot)
        *(.text)
    }

    .rodata ALIGN (0x1000) : AT(ADDR(.rodata))
    {
        *(.rodata*)
    }

    .data ALIGN (0x1000) : AT(ADDR(.data))
    {
        *(.data)
    }

    .bss ALIGN (0x1000) : AT(ADDR(.bss))
    {
        *(COMMON)
        *(.bss)
    }
}