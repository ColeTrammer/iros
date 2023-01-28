#include <netdb.h>

extern int __net_stayopen;

struct netent *getnetbyaddr(uint32_t addr, int addrtype) {
    struct netent *net;
    setnetent(__net_stayopen);
    while ((net = getnetent())) {
        if (net->n_net == addr && net->n_addrtype == addrtype) {
            break;
        }
    }

    if (!__net_stayopen) {
        endnetent();
    }
    return net;
}
