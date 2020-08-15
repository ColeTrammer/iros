#include <netdb.h>
#include <stdio.h>

extern FILE *__hostent_file;

void sethostent(int keepopen) {
    (void) keepopen;

    if (!__hostent_file) {
        __hostent_file = fopen("/etc/hosts", "r");
    } else {
        rewind(__hostent_file);
    }
}
