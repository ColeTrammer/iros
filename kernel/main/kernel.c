#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <kernel/display/terminal.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/mem/page_frame_allocator.h>

void kernel_main(uint64_t kernel_phys_start, uint64_t kernel_phys_end, uint32_t *multiboot_info) {
    set_background(VGA_COLOR_BLACK);
    set_foreground(VGA_COLOR_LIGHT_GREY);
    clear_terminal();

    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, multiboot_info);
    init_interrupts();

    while (1);
}