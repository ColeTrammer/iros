#include <netdb.h>

static struct protoent __static_protoent = { 0 };
static char __static_protoent_buffer[1024] = { 0 };

struct protoent *getprotoent(void) {
    struct protoent *ret;
    getprotoent_r(&__static_protoent, __static_protoent_buffer, sizeof(__static_protoent_buffer), &ret);
    return ret;
}
