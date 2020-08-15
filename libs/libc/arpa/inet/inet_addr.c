#include <arpa/inet.h>

in_addr_t inet_addr(const char *cp) {
    struct in_addr addr;
    int ret = inet_aton(cp, &addr);
    if (ret == 0) {
        return INADDR_NONE;
    }
    return addr.s_addr;
}
