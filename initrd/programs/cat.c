#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

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

    struct stat stat_struct;
    memset(&stat_struct, 0, sizeof(struct stat));
    if (stat(argv[1], &stat_struct) != 0) {
        perror("cat (stat)");
        return 1;
    }

    char *buffer = malloc(stat_struct.st_size + 1);
    int read = fread(buffer, 1, stat_struct.st_size + 1, file);
    if (read == 0) {
        perror("cat");
        return 1;
    }
    printf("%s", buffer);
    fclose(file);

    return 0;
}