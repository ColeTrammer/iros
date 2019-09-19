#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    /* Clear using terminal escaped strings */

    printf("%s%s", "\033[0;0H", "\033[2J");
    fflush(stdout);

    return 0;
}