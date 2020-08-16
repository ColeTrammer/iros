#include <netdb.h>

static struct servent __static_servent = { 0 };
static char __static_servent_buffer[1024] = { 0 };

struct servent *getservent(void) {
    struct servent *ret;
    getservent_r(&__static_servent, __static_servent_buffer, sizeof(__static_servent_buffer), &ret);
    return ret;
}
