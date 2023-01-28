#include <netdb.h>
#include <string.h>

extern int __serv_stayopen;

struct servent *getservbyport(int port, const char *proto) {
    struct servent *serv;
    setservent(__serv_stayopen);
    while ((serv = getservent())) {
        if (serv->s_port == port && (!proto || strcmp(serv->s_proto, proto) == 0)) {
            break;
        }
    }

    if (!__serv_stayopen) {
        endservent();
    }
    return serv;
}
