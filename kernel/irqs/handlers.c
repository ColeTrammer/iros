#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <kernel/hal/output.h>
#include <kernel/irqs/handlers.h>
#include <kernel/hal/irqs.h>
#include <kernel/hal/output.h>

void init_irq_handlers() {
    register_irq_handler(&handle_double_fault_entry, 8, false);
    register_irq_handler(&handle_general_protection_fault_entry, 13, false);
    register_irq_handler(&handle_page_fault_entry, 14, false);

    register_irq_handler(&sys_call_entry, 128, true);

    debug_log("Finished Initializing Handlers\n");
}

void handle_double_fault() {
    dump_registers_to_screen();
    printf("\n\033[31m%s\033[0m\n", "Double Fault");
    abort();
}

void handle_general_protection_fault(uintptr_t error) {
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error %#lX\033[0m\n", "General Protection Fault", error);
    abort();
}

void handle_page_fault(uintptr_t address, uintptr_t error) {
    dump_registers_to_screen();
    printf("\n\033[31m%s: Error %lX\n", "Page Fault", error);
    printf("Address: %#.16lX\033[0m\n", address);
    abort();
}