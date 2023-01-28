#include <netdb.h>
#include <stdio.h>

FILE *__serv_file = NULL;
int __serv_stayopen = 0;

void setservent(int stayopen) {
    __serv_stayopen = stayopen;
    if (__serv_file) {
        rewind(__serv_file);
    } else {
        __serv_file = fopen("/etc/services", "r");
    }
}
