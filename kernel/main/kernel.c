#include <stdint.h>

void kernel_main() {
    __asm__("mov $0xBAD2B0, %edx");
    while (1);
}