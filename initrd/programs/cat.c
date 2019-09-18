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

    char *buffer = malloc(512);
    int read = fread(buffer, 1, 512, file);
    if (read == 0) {
        perror("cat");
        return 1;
    }
    printf("%s", buffer);
    fclose(file);

    return 0;
}