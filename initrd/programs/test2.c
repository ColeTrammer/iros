#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void yield() {
    asm ( "mov $2, %%rdi\n"\
          "int $0x80" : : : "rdi" );
}

int main() {
    for (int i = 0; i < 12; i++) {
        puts("Test 2");
        if (i % 5 == 0) {
            yield();
        }
    }

    return EXIT_SUCCESS;
}