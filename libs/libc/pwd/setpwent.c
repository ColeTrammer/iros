#include <pwd.h>
#include <stdio.h>

extern FILE *__pwd_file;

void setpwent(void) {
    if (__pwd_file) {
        rewind(__pwd_file);
    }
}
