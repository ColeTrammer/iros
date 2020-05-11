#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static const char *message = "mmap test!";
static const char *path = ".mmap_file_test";

static void cleanup_file(void) {
    if (unlink(path)) {
        perror("unlink");
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc == 2) {
        message = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s  [message]\n", argv[0]);
        return 2;
    }

    size_t message_length = strlen(message);

    char buffer[2048];
    snprintf(buffer, sizeof(buffer) - 1, "echo -n '%.512s' > %.512s", message, path);
    if (system(buffer)) {
        fprintf(stderr, "system failed\n");
        return 1;
    }

    atexit(cleanup_file);

    FILE *file = fopen(path, "r");
    if (!file) {
        perror("fopen");
        return 1;
    }

    char *mapped_message = mmap(NULL, message_length, PROT_READ, MAP_SHARED, fileno(file), 0);
    if (mapped_message == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    printf("%.*s\n", (int) message_length, mapped_message);

    if (munmap(mapped_message, message_length)) {
        perror("munmap");
        return 1;
    }

    if (fclose(file)) {
        perror("fclose");
        return 1;
    }

    return 0;
}
