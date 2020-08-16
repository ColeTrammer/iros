#include <netdb.h>

static struct netent __static_netent = { 0 };
static char __static_netent_buffer[1024] = { 0 };

struct netent *getnetent(void) {
    struct netent *ret;
    getnetent_r(&__static_netent, __static_netent_buffer, sizeof(__static_netent_buffer), &ret);
    return ret;
}
