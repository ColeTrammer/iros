/* Sets first thing to be executed to be start */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x400000)); 
    . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;

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
    .rodata :
    {
        PROVIDE(_rod_start = .);
        *(.rodata*)
        PROVIDE(_rod_end = .);
    }

    /* Put data */
    .data :
    {
        PROVIDE(_data_start = .);
        *(.data)
        PROVIDE(_data_end = .);
    }

    /* Put uninitialized data */
    .bss :
    {
        PROVIDE(_bss_start = .);
        *(COMMON)
        *(.bss .bss.*)
        PROVIDE(_bss_end = .);
    }

    /DISCARD/ : { *(.comment) }
}