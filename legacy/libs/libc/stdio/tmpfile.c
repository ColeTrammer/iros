#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

FILE *tmpfile(void) {
    size_t failed_attempts = 0;
    while (failed_attempts < TMP_MAX) {
        char name[L_tmpnam];
        tmpnam(name);

        if (access(name, F_OK) != -1) {
            failed_attempts++;
            continue;
        }

        return fopen(name, "w+");
    }

    // Give up
    return NULL;
}
