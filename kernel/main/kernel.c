#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include <kernel/display/terminal.h>
#include <kernel/interrupts/interrupts.h>

void kernel_main(uint32_t *multiboot_info) {
    set_background(VGA_COLOR_BLACK);
    set_foreground(VGA_COLOR_LIGHT_GREY);
    clear_terminal();

    init_interrupts();
    printf("Multiboot: %#.16lX\n", multiboot_info);
    printf("Multiboot Size: %d\n", multiboot_info[0]);
    uint32_t *data = multiboot_info + 2;
    while (data < multiboot_info + multiboot_info[0] / sizeof(uint32_t)) {
        printf("Type: %d\n", data[0]);
        if (data[0] == 4) {
            printf("Mem Lower: %d\n", data[2]);
            printf("Mem Upper: %d\n", data[3]);
        }
        if (data[0] == 6) {
            uint64_t *mem = (uint64_t*) (data + 4);
            while ((uint32_t*) mem < data + data[1] / sizeof(uint32_t)) {
                printf("Addr: %#.16lX  Length: %#.16lX  Type: %d\n", mem[0], mem[1], (uint32_t) mem[2]);
                mem += data[2] / sizeof(uint64_t);
            }
        }
        data = (uint32_t*) ((uint64_t) data + data[1]);
        if ((uint64_t) data % 8 != 0) {
                data = (uint32_t*) (((uint64_t) data & ~0x7) + 8);
            }
    }
    while (1);
}