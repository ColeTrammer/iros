#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void yield() {
    asm ( "mov $2, %%rdi\n"\
          "int $0x80" : : : "rdi" );
}

int main() {
    for (int i = 0; i < 7; i++) {
        puts("Test 1");
        if (i % 2 == 0) {
            yield();
        }
    }

    return EXIT_SUCCESS;
}