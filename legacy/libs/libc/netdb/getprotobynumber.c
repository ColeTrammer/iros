#include <netdb.h>

extern int __proto_stayopen;

struct protoent *getprotobynumber(int protocol) {
    struct protoent *proto;
    setprotoent(__proto_stayopen);
    while ((proto = getprotoent())) {
        if (proto->p_proto == protocol) {
            break;
        }
    }

    if (!__proto_stayopen) {
        endprotoent();
    }
    return proto;
}
