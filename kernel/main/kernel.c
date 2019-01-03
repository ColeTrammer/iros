#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/display/terminal.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>

void kernel_main(uint64_t kernel_phys_start, uint64_t kernel_phys_end, uint32_t *multiboot_info) {
    init_terminal();
    init_interrupts();
    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, multiboot_info);
    init_vm_allocator(kernel_phys_start, kernel_phys_end);

    while (1);
}