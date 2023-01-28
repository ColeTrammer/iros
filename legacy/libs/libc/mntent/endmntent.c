#include <mntent.h>
#include <stdio.h>

int endmntent(FILE *file) {
    return fclose(file);
}
