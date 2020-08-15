#include <netdb.h>
#include <stdio.h>

FILE *__proto_file = NULL;
int __proto_stayopen = 0;

void setprotoent(int stayopen) {
    __proto_stayopen = stayopen;
    if (__proto_file) {
        rewind(__proto_file);
    } else {
        __proto_file = fopen("/etc/protocols", "r");
    }
}
