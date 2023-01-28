#include <netdb.h>
#include <string.h>

extern int __net_stayopen;

struct netent *getnetbyname(const char *name) {
    struct netent *net;
    setnetent(__net_stayopen);
    while ((net = getnetent())) {
        if (strcmp(net->n_name, name) == 0) {
            break;
        }

        for (size_t i = 0; net->n_aliases[i]; i++) {
            if (strcmp(net->n_aliases[i], name) == 0) {
                goto done;
            }
        }
    }

done:
    if (!__net_stayopen) {
        endnetent();
    }
    return net;
}
