#include <netdb.h>
#include <stdio.h>

extern FILE *__proto_file;

void endprotoent(void) {
    if (__proto_file) {
        fclose(__proto_file);
        __proto_file = NULL;
    }
}
