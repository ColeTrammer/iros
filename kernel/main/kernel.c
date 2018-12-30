#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <kernel/display/terminal.h>

void kernel_main() {
    set_background(VGA_COLOR_BLACK);
    set_foreground(VGA_COLOR_LIGHT_GREY);
    clear_terminal();
    printf("%s\n", "Hello World!");
    while (1);
}