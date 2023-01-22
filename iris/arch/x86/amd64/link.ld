OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

ENTRY(iris_entry)

PHDRS
{
    text    PT_LOAD    FLAGS((1 << 0) | (1 << 2)) ; /* Execute + Read */
    rodata  PT_LOAD    FLAGS((1 << 2)) ;            /* Read only */
    data    PT_LOAD    FLAGS((1 << 1) | (1 << 2)) ; /* Write + Read */
}

SECTIONS
{
    /* The Higher-Half kernel is mapped at -2 GiB for -mcmodel=kernel GCC optimization */
    . = 0xffffffff80000000;

    PROVIDE(__iris_kernel_start = .);
    PROVIDE(__iris_text_segment_start = .);
    .text : {
        *(.text .text.*)
    } :text

    /* Move to the next memory page for .rodata */
    . += CONSTANT(MAXPAGESIZE);
    PROVIDE(__iris_text_segment_end = .);

    PROVIDE(__iris_rodata_segment_start = .);
    .rodata : {
        *(.rodata .rodata.*)

        /* The provided addresses for these arrays must be 8 byte aligned, or else there will be unfortunate padding. */
        . = ALIGN(., 8);
        PROVIDE(__iris_init_array_start = .);
        *(.init_array)
        PROVIDE(__iris_init_array_end = .);

        . = ALIGN(., 8);
        PROVIDE(__iris_fini_array_start = .);
        *(.fini_array)
        PROVIDE(__iris_fini_array_end = .);
    } :rodata

    /* Move to the next memory page for .data */
    . += CONSTANT(MAXPAGESIZE);
    PROVIDE(__iris_rodata_segment_end = .);

    PROVIDE(__iris_data_segment_start = .);
    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(COMMON)
        *(.bss .bss.*)
    } :data

    /* Move to the next memory page for alignment */
    . += CONSTANT(MAXPAGESIZE);
    PROVIDE(__iris_data_segment_end = .);
    PROVIDE(__iris_kernel_end = .);

    /* Discard .note.* and .eh_frame since they may cause issues on some hosts. */
    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}