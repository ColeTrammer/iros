#include <stdint.h>
#include <stddef.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BASE ((uint16_t*) 0xB8000)
#define VGA_INDEX(row, col) ((row) * VGA_WIDTH + (col))
#define VGA_ENTRY(c, fg, bg) \
    (((uint16_t) (c) & 0x00FF) | ((uint16_t) (fg) << 8 & 0x0F00) | ((uint16_t) (bg) << 12 & 0xF000))

void clear_screen() {
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BASE[VGA_INDEX(row, col)] = VGA_ENTRY(' ', 15, 0);
        }
    }
}

static size_t row = 0;

void kprint(const char *str) {
    if (row >= VGA_HEIGHT) {
        row = 0;
    }
    for (size_t i = 0; str[i] != '\0'; i++) {
        VGA_BASE[VGA_INDEX(row, i)] = VGA_ENTRY(str[i], 7, 0);
    }
    row++;
}

void kernel_main() {
    clear_screen();
    kprint("Hello World!");
    while (1);
}