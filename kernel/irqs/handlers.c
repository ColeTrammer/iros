#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <kernel/display/vga.h>
#include <kernel/display/terminal.h>
#include <kernel/irqs/handlers.h>
#include <kernel/hal/irqs.h>

void init_irq_handlers() {
    register_irq_handler(&handle_double_fault_entry, 8, false);
    register_irq_handler(&handle_general_protection_fault_entry, 13, false);
    register_irq_handler(&handle_page_fault_entry, 14, false);

    register_irq_handler(&sys_call_entry, 128, true);
}

void handle_double_fault() {
    set_foreground(VGA_COLOR_RED);
    printf("%s\n", "Double Fault");
    dump_registers();
    abort();
}

void handle_general_protection_fault(uintptr_t error) {
    set_foreground(VGA_COLOR_RED);
    printf("%s: Error %#lX\n", "General Protection Fault", error);
    dump_registers();
    abort();
}

void handle_page_fault(uintptr_t address, uintptr_t error) {
    set_foreground(VGA_COLOR_RED);
    printf("%s: Error %lX\n", "Page Fault", error);
    printf("Address: %#.16lX\n", address);
    dump_registers();
    abort();
}

void sys_call() {
    puts("Sys Call");
}