#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <file-name>\n", argv[0]);
        return 0;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        perror("cat");
        return 1;
    }

    char *buffer = malloc(0x5000);
    fread(buffer, 1, 0x5000, file);
    puts(buffer);
    fclose(file);

    return 0;
}