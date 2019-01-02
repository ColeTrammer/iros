#include <stdio.h>
#include <stdlib.h>

#include <kernel/display/vga.h>
#include <kernel/display/terminal.h>
#include <kernel/interrupts/interrupts.h>

void handle_double_fault() {
    set_foreground(VGA_COLOR_RED);
    printf("%s\n", "Double Fault");
    dump_registers();
    abort();
}

void handle_page_fault(uint64_t address, uint64_t error) {
    set_foreground(VGA_COLOR_RED);
    printf("%s: Error %lX\n", "Page Fault", error);
    printf("Address: %#.16lX\n", address);
    dump_registers();
    abort();
}