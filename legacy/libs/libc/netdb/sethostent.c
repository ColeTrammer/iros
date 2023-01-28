#include <netdb.h>
#include <stdio.h>

FILE *__hostent_file = NULL;

void sethostent(int keepopen) {
    (void) keepopen;

    if (!__hostent_file) {
        __hostent_file = fopen("/etc/hosts", "r");
    } else {
        rewind(__hostent_file);
    }
}
