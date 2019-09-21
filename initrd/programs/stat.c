#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return EXIT_SUCCESS;
    }

    struct stat stat_s;
    memset(&stat_s, 0, sizeof(struct stat));
    int ret = stat(argv[1], &stat_s);
    if (ret != 0) {
        perror("Stat");
        return EXIT_FAILURE;
    }

    printf("%6s: %s\n", "File", argv[1]);
    printf("%6s: %-15ld %s: %-10ld %s: %-6ld %s\n", "Size", stat_s.st_size, "Blocks", stat_s.st_blocks, "IO Block", stat_s.st_blksize, "regular file");
    printf("%6s: %luh/%lud %s: %-7lld %s: %d\n", "Device", stat_s.st_dev, stat_s.st_rdev, "Inode", stat_s.st_ino, "Links", stat_s.st_nlink);

    return EXIT_SUCCESS;
}