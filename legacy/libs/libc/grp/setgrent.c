#include <grp.h>
#include <stdio.h>

extern FILE *__group_file;

void setgrent(void) {
    if (__group_file) {
        rewind(__group_file);
    }
}
