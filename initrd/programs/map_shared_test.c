#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    assert(argc == 2);

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct stat stat_buf;
    if (fstat(fd, &stat_buf)) {
        perror("stat");
        return 1;
    }

    void *file = mmap(NULL, stat_buf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    char *buffer = calloc(stat_buf.st_size + 1, sizeof(char));
    if (!buffer) {
        perror("malloc");
        return 1;
    }

    memcpy(buffer, file, stat_buf.st_size);
    fputs(buffer, stdout);

    if (munmap(file, stat_buf.st_size)) {
        perror("munmap");
        return 1;
    }

    return 0;
}