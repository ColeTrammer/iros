#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/hal/output.h>

static uint16_t *vga_buffer = (uint16_t*) VGA_PHYS_ADDR;
static enum vga_color fg = VGA_COLOR_LIGHT_GREY;
static enum vga_color bg = VGA_COLOR_BLACK;

void set_vga_buffer(uint16_t *_vga_buffer) {
    vga_buffer = _vga_buffer;
    debug_log("VGA Buffer Changed: [ %#.16lX ]\n", _vga_buffer);
}

void set_vga_foreground(enum vga_color _fg) {
    fg = _fg;
}

void set_vga_background(enum vga_color _bg) {
    bg = _bg;
}

void write_vga_buffer(size_t row, size_t col, char c) {
    vga_buffer[VGA_INDEX(row, col)] = VGA_ENTRY(c, fg, bg);
}

uint16_t get_vga_buffer(size_t row, size_t col) {
    return vga_buffer[VGA_INDEX(row, col)];
}