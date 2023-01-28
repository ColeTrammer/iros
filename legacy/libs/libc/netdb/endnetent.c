#include <netdb.h>
#include <stdio.h>

extern FILE *__net_file;

void endnetent(void) {
    if (__net_file) {
        fclose(__net_file);
        __net_file = NULL;
    }
}
