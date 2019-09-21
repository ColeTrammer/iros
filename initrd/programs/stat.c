#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *type_to_string(mode_t mode) {
    if (S_ISBLK(mode)) {
        return "block special file";
    } else if (S_ISCHR(mode)) {
        return "character special file";
    } else if (S_ISFIFO(mode)) {
        return "fifo special file";
    } else if (S_ISLNK(mode)) {
        return "symbolic link";
    } else if (S_ISREG(mode)) {
        return "regular file";
    } else {
        return "unknown file type";
    }
}

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
    printf("%6s: %-14ld %s: %-9ld %s: %-6ld %s\n", "Size", stat_s.st_size, "Blocks", stat_s.st_blocks, "IO Block", stat_s.st_blksize, type_to_string(stat_s.st_mode));
    printf("%6s: %luh/%lud %s: %-7lld %s: %d\n", "Device", stat_s.st_dev, stat_s.st_rdev, "Inode", stat_s.st_ino, "Links", stat_s.st_nlink);

    return EXIT_SUCCESS;
}