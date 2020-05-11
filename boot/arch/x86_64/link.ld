/* Sets first thing to be executed to be start */
ENTRY(start)

SECTIONS
{
    PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x200000)); 
    . = SEGMENT_START("text-segment", 0x200000) + SIZEOF_HEADERS;

    /* Put .multiboot first, then .text */
    .init :
    {
        *(.multiboot)
        PROVIDE(_text_start = .);
        *(SORT_NONE(.init))
    }
    
    .text :
    {
        *(.text)
    }

    .fini :
    {
        *(SORT_NONE(.fini))
        PROVIDE(_text_end = .);
    }

    /* Put read only data */
    .rodata :
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
    .data :
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
    .bss :
    {
        PROVIDE(_bss_start = .);
        *(COMMON)
        *(.bss)
        PROVIDE(_bss_end = .);
    }

    /DISCARD/ : { *(.comment) }
}
