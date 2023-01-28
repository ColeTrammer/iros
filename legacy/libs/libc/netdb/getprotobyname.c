#include <netdb.h>
#include <string.h>

extern int __proto_stayopen;

struct protoent *getprotobyname(const char *name) {
    struct protoent *proto;
    setprotoent(__proto_stayopen);
    while ((proto = getprotoent())) {
        if (strcmp(proto->p_name, name) == 0) {
            break;
        }

        for (size_t i = 0; proto->p_aliases[i]; i++) {
            if (strcmp(proto->p_aliases[i], name) == 0) {
                goto done;
            }
        }
    }

done:
    if (!__proto_stayopen) {
        endprotoent();
    }
    return proto;
}
