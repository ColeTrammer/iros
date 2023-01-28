#include <pwd.h>
#include <stdio.h>

extern FILE *__pwd_file;

void endpwent(void) {
    if (__pwd_file) {
        fclose(__pwd_file);
        __pwd_file = NULL;
    }
}
