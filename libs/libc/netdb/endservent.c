#include <netdb.h>
#include <stdio.h>

extern FILE *__serv_file;

void endservent(void) {
    if (__serv_file) {
        fclose(__serv_file);
        __serv_file = NULL;
    }
}
