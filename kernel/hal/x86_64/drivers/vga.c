#include <stdbool.h>

#include <kernel/mem/page.h>
#include <kernel/mem/vm_region.h>
#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/output.h>

static uint16_t *vga_buffer = (uint16_t*) VGA_PHYS_ADDR;
static enum vga_color fg = VGA_COLOR_LIGHT_GREY;
static enum vga_color bg = VGA_COLOR_BLACK;

static void enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, (inb(VGA_DATA) & 0xC0) | cursor_start);
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_END, (inb(VGA_DATA) & 0xE0) | cursor_end);
}

static void disable_cursor() {
	VGA_RUN_COMMAND(VGA_ENABLE_CURSOR_START, VGA_CURSOR_DISABLE);
}

void update_vga_buffer() {
    map_phys_page(VGA_PHYS_ADDR & ~0xFFF, VGA_VIRT_ADDR, VM_NO_EXEC | VM_GLOBAL | VM_WRITE);
    vga_buffer = (uint16_t*) VGA_VIRT_ADDR;
    debug_log("VGA Buffer Updated: [ %#.16lX ]\n", VGA_VIRT_ADDR);

    enable_cursor(VGA_CURSOR_Y_START, VGA_CURSOR_Y_END);
}

void set_vga_foreground(enum vga_color _fg) {
    fg = _fg;
}

void set_vga_background(enum vga_color _bg) {
    bg = _bg;
}

void write_vga_buffer(size_t row, size_t col, uint16_t c, bool raw_copy) {
    vga_buffer[VGA_INDEX(row, col)] = raw_copy ? c : VGA_ENTRY(c & 0xFF, fg, bg);
}

uint16_t get_vga_buffer(size_t row, size_t col) {
    return vga_buffer[VGA_INDEX(row, col)];
}

void set_vga_cursor(size_t row, size_t col) {
    uint16_t pos = (uint16_t) VGA_INDEX(row, col);

    if (pos >= VGA_WIDTH * VGA_HEIGHT) {
        disable_cursor();
        return;
    }

    VGA_RUN_COMMAND(VGA_SET_CURSOR_LOW, (uint8_t) (pos & 0xFF));
    VGA_RUN_COMMAND(VGA_SET_CURSOR_HIGH, (uint8_t) ((pos >> 8) & 0xFF));
}