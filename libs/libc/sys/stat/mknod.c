#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/stat.h>

int mknod(const char *path, mode_t mode, dev_t dev) {
    (void) path;
    (void) mode;
    (void) dev;

    fprintf(stderr, "mknod not supported\n");
    assert(false);
    return 0;
}