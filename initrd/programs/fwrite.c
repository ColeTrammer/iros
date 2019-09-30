#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <file-name> <text>\n", argv[0]);
        return 0;
    }

    FILE *f = fopen(argv[1], "w");
    if (!f) {
        perror("fwrite");
        return 1;
    }

    size_t len = strlen(argv[2]) + 1;
    if (fputs(argv[2], f) < (int) len) {
        perror("fwrite");
        return 1;
    }

    if (fclose(f) == EOF) {
        perror("fwrite");
        return 1;
    }

    return 0;
}