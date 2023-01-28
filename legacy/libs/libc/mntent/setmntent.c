#include <mntent.h>
#include <stdio.h>

FILE *setmntent(const char *filename, const char *type) {
    return fopen(filename, type);
}
