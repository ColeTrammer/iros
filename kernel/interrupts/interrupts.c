#include <stdio.h>
#include <stdlib.h>

#include <kernel/display/vga.h>
#include <kernel/display/terminal.h>
#include <kernel/interrupts/interrupts.h>

void handle_double_fault() {
    set_foreground(VGA_COLOR_LIGHT_RED);
    printf("%s\n", "Double Fault");
    dump_registers();
    abort();
}