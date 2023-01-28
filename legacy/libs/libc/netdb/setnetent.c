#include <netdb.h>
#include <stdio.h>

FILE *__net_file = NULL;
int __net_stayopen = 0;

void setnetent(int stayopen) {
    __net_stayopen = stayopen;
    if (__net_file) {
        rewind(__net_file);
    } else {
        __net_file = fopen("/etc/networks", "r");
    }
}
