OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

/* Sets first thing to be executed to be start */
ENTRY(_start)

SECTIONS
{
    . = 0x400000 + SIZEOF_HEADERS;

    .text :
    {
        KEEP(*(SORT_NONE(.init)))
        *(.text.startup)
        *(.text .text.*)
        KEEP(*(SORT_NONE(.fini)))
    }

    /* Put read only data */
    .rodata ALIGN(0x1000) :
    {
        *(.rodata .rodata.*)
        *(.eh_frame)
    }

    /* Put data */
    .data ALIGN(0x1000) :
    {
        *(.data)
        *(.ctors)
        *(.dtors)
    }

    /* Put uninitialized data */
    .bss ALIGN(0x1000) :
    {
        *(COMMON)
        *(.bss .bss.*)
    }

    /DISCARD/ : { 
        *(.comment)
        *(.debug*)
    }
}
