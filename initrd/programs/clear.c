#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    /* Clear hack (should send something directly to tty) (also doesn't even work since it sets cursor to bottom) */
    for (size_t i = 0; i < 25; i++) {
        puts(" ");
    }

    return 0;
}