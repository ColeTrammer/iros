#include <grp.h>
#include <stddef.h>
#include <stdio.h>

extern FILE *__group_file;

void endgrent(void) {
    if (__group_file) {
        fclose(__group_file);
        __group_file = NULL;
    }
}
