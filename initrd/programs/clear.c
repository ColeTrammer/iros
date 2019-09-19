#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    /* Clear using terminal escaped strings */

    printf("%s%s", "\x1B[0;0H", "\x1B[2J");

    return 0;
}