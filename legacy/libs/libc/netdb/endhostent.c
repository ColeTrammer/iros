#include <netdb.h>
#include <stdio.h>

extern FILE *__hostent_file;

void endhostent(void) {
    if (__hostent_file) {
        fclose(__hostent_file);
        __hostent_file = NULL;
    }
}
