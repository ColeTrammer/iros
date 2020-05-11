#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <file-name>\n", argv[0]);
        return 0;
    }

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        perror("touch");
        return 1;
    }

    if (fclose(f) == EOF) {
        perror("touch");
        return 1;
    }

    return 0;
}
