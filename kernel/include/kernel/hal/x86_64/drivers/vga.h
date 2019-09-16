#ifndef _KERNEL_HAL_X86_64_DRIVERS_VGA_H
#define _KERNEL_HAL_X86_64_DRIVERS_VGA_H 1

#include <stddef.h>
#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>

extern void _reserved_vga_buffer();

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_PHYS_ADDR 0xB8000
#define VGA_VIRT_ADDR ((uintptr_t) &_reserved_vga_buffer)
#define VGA_INDEX(row, col) ((row) * VGA_WIDTH + (col))
#define VGA_ENTRY(c, fg, bg) (((uint16_t) (c) & 0x00FF) | ((uint16_t) (fg) << 8 & 0x0F00) | ((uint16_t) (bg) << 12 & 0xF000))

#define VGA_CURSOR_Y_START 14
#define VGA_CURSOR_Y_END 15

#define VGA_COMMAND 0x3D4
#define VGA_DATA 0x3D5
#define VGA_SET_CURSOR_LOW 0x0F
#define VGA_SET_CURSOR_HIGH 0x0E
#define VGA_ENABLE_CURSOR_START 0x0A
#define VGA_ENABLE_CURSOR_END 0x0B
#define VGA_CURSOR_DISABLE 0x20
#define VGA_RUN_COMMAND(p1, p2)  \
	do {                         \
		outb(VGA_COMMAND, (p1)); \
		outb(VGA_DATA, (p2));    \
	} while (0);

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

void update_vga_buffer();
void set_vga_foreground(enum vga_color fs);
void set_vga_background(enum vga_color bg);

void write_vga_buffer(size_t row, size_t col, char c);
uint16_t get_vga_buffer(size_t row, size_t col);

void set_vga_cursor(size_t row, size_t col);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_VGA_H */