OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

PROVIDE(KERNEL_VMA = 0xFFFFFFFF80000000);
PROVIDE(KERNEL_PHYS_START = 0x200000);

/* Sets first thing to be executed to be start */
ENTRY(_start32)

SECTIONS
{
    PROVIDE (__executable_start = SEGMENT_START("text-segment", KERNEL_VMA)); 
    . = SEGMENT_START("text-segment", KERNEL_VMA) + SIZEOF_HEADERS;

    .init : AT(ADDR(.init) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        PROVIDE(_kernel_phys_start = . - KERNEL_VMA + KERNEL_PHYS_START - SIZEOF_HEADERS);
        PROVIDE(_multiboot_header_start_boot = . - KERNEL_VMA + KERNEL_PHYS_START);
        *(.multiboot)
        PROVIDE(_text_start = .);
        PROVIDE(_text_start_boot = . - KERNEL_VMA + KERNEL_PHYS_START);
        KEEP(*(SORT_NONE(.init)))

        PROVIDE(__init_level_fs_start = .);
        KEEP(*(SORT_NONE(.init.level.fs)))
        PROVIDE(__init_level_fs_end = .);

        PROVIDE(__init_level_time_start = .);
        KEEP(*(SORT_NONE(.init.level.time)))
        PROVIDE(__init_level_time_end = .);

        PROVIDE(__init_level_driver_start = .);
        KEEP(*(SORT_NONE(.init.level.driver)))
        PROVIDE(__init_level_driver_end = .);

        PROVIDE(__init_level_net_start = .);
        KEEP(*(SORT_NONE(.init.level.net)))
        PROVIDE(__init_level_net_end = .);
    }

    .text : AT(ADDR(.text) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        PROVIDE(_kernel_entry32 = . - KERNEL_VMA + KERNEL_PHYS_START);
        *(.text.startup)
        PROVIDE(_kernel_ap_trampoline_start = .);
        *(.text.ap_boot)
        PROVIDE(_kernel_ap_trampoline_end = .);
        *(.text .text.*)
    }

    .fini : AT(ADDR(.fini) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        KEEP(*(SORT_NONE(.fini)))
        PROVIDE(_text_end = .);
    }

    /* Put read only data */
    .rodata ALIGN(0x1000) : AT(ADDR(.rodata) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        PROVIDE(_rod_start = .);
        *(.rodata .rodata.*)
    }

    .ctors : AT(ADDR(.ctors) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        *(.ctors)
    }

    .dtors : AT(ADDR(.dtors) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        *(.dtors)
    }

    .eh_frame : AT(ADDR(.eh_frame) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        *(.eh_frame)
        PROVIDE(_rod_end = .);
    }

    /* Put data */
    .data ALIGN(0x1000) : AT(ADDR(.data) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        PROVIDE(_data_start = .);
        *(.data)
        PROVIDE(_data_end_boot = . - KERNEL_VMA + KERNEL_PHYS_START);
        PROVIDE(_data_end = .);
    }

    /* Put uninitialized data */
    .bss ALIGN(0x1000) : AT(ADDR(.bss) - KERNEL_VMA + KERNEL_PHYS_START)
    {
        PROVIDE(_bss_start = .);
        *(.bss.kernel_stack)
        *(.bss.reserved_pages)
        *(COMMON)
        *(.bss .bss.*)
        PROVIDE(_bss_end_boot = . - KERNEL_VMA + KERNEL_PHYS_START);
        PROVIDE(_bss_end = .);
        PROVIDE(_kernel_phys_end = . - KERNEL_VMA + KERNEL_PHYS_START);
    }

    /DISCARD/ : { *(.comment) }
}
