#include <stdio.h>
#include <sys/stat.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <dir-name>]n", argv[0]);
        return 0;
    }

    int ret = mkdir(argv[1], 0777);
    if (ret != 0) {
        perror("mkdir");
        return 1;
    }

    return 0;
}