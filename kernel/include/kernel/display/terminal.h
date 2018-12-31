#ifndef _KERNEL_DISPLAY_TERNINAL_H
#define _KERNEL_DISPLAY_TERNINAL_H 1

#include <stdbool.h>
#include <stddef.h>

#include <kernel/display/vga.h>

void dump_registers();

void clear_terminal();
bool kprint(const char *str, size_t len);
void set_foreground(enum vga_color foreground);
void set_background(enum vga_color gackground);

#endif /* _KERNEL_DISPLAY_TERMINAL_H */