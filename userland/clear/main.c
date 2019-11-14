#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 1) {
        printf("Usage: %s\n", argv[0]);
        return 0;
    }

    /* Clear using terminal escaped strings */

    printf("%s%s", "\033[1;1H", "\033[3J");
    fflush(stdout);

    return 0;
}