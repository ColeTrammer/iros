#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <kernel/display/terminal.h>
#include <kernel/irqs/handlers.h>
#include <kernel/mem/page_frame_allocator.h>
#include <kernel/mem/vm_allocator.h>

#include <hal/hal.h>

void kernel_main(uintptr_t kernel_phys_start, uintptr_t kernel_phys_end, uint32_t *multiboot_info) {
    init_terminal();
    init_hal();
    init_irq_handlers();
    init_page_frame_allocator(kernel_phys_start, kernel_phys_end, multiboot_info);
    init_vm_allocator(kernel_phys_start, kernel_phys_end);
    init_drivers();

    enable_interrupts();

    while (1);
}