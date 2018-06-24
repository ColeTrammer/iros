/* Sets first thing to be executed to be start */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

KERNEL_VMA = 0x400000;

ENTRY(_start)

SECTIONS
{
    PROVIDE (__executable_start = SEGMENT_START("text-segment", KERNEL_VMA)); 
    . = SEGMENT_START("text-segment", KERNEL_VMA) + SIZEOF_HEADERS;

    .init :
    {
        PROVIDE(_text_start = .);
        KEEP(*(SORT_NONE(.init)))
    }

    .text :
    {
        *(.text.startup)
        *(.text .text.*)
    }

    .fini :
    {
        KEEP(*(SORT_NONE(.fini)))
        PROVIDE(_text_end = .);
    }

    /* Put read only data */
    .rodata ALIGN(0x1000) :
    {
        PROVIDE(_rod_start = .);
        *(.rodata*)
    }

    .eh_frame :
    {
        *(.eh_frame)
        PROVIDE(_rod_end = .);
    }

    /* Put data */
    .data ALIGN(0x1000) :
    {
        PROVIDE(_data_start = .);
        *(.data)
    }

    .ctors :
    {
        *(.ctors)
    }

    .dtors :
    {
        *(.dtors)
        PROVIDE(_data_end = .);
    }

    /* Put uninitialized data */
    .bss ALIGN(0x1000) :
    {
        PROVIDE(_bss_start = .);
        *(COMMON)
        *(.bss .bss.*)
        PROVIDE(_bss_end = .);
    }

    /DISCARD/ : { *(.comment) }
}