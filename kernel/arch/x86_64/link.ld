OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

KERNEL_VMA = 0xFFFFFFF800000000;

/* Sets first thing to be executed to be start */
ENTRY(_start)

SECTIONS
{
    PROVIDE (__executable_start = SEGMENT_START("text-segment", KERNEL_VMA)); 
    . = SEGMENT_START("text-segment", KERNEL_VMA) + SIZEOF_HEADERS;

    .init : AT(ADDR(.init) - KERNEL_VMA)
    {
        PROVIDE(_text_start = .);
        KEEP(*(SORT_NONE(.init)))
    }

    .text : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text.startup)
        *(.text .text.*)
    }

    .fini : AT(ADDR(.fini) - KERNEL_VMA)
    {
        KEEP(*(SORT_NONE(.fini)))
        PROVIDE(_text_end = .);
    }

    /* Put read only data */
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        PROVIDE(_rod_start = .);
        *(.rodata*)
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA)
    {
        *(.eh_frame)
        PROVIDE(_rod_end = .);
    }

    /* Put data */
    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VMA)
    {
        PROVIDE(_data_start = .);
        *(.data)
    }

    .ctors : AT(ADDR(.ctors) - KERNEL_VMA)
    {
        *(.ctors)
    }

    .dtors : AT(ADDR(.dtors) - KERNEL_VMA)
    {
        *(.dtors)
        PROVIDE(_data_end = .);
    }

    /* Put uninitialized data */
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        PROVIDE(_bss_start = .);
        *(COMMON)
        *(.bss .bss.*)
        PROVIDE(_bss_end = .);
    }

    /DISCARD/ : { *(.comment) }
}