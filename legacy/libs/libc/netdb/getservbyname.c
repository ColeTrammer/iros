#include <netdb.h>
#include <string.h>

extern int __serv_stayopen;

struct servent *getservbyname(const char *name, const char *protocol) {
    struct servent *serv;
    setservent(__serv_stayopen);
    while ((serv = getservent())) {
        if (!!protocol && strcmp(serv->s_proto, protocol) != 0) {
            continue;
        }

        if (strcmp(serv->s_name, name) == 0) {
            break;
        }

        for (size_t i = 0; serv->s_aliases[i]; i++) {
            if (strcmp(serv->s_aliases[i], name) == 0) {
                goto done;
            }
        }
    }

done:
    if (!__serv_stayopen) {
        endservent();
    }
    return serv;
}
