#include <arpa/inet.h>
#include <stdio.h>

int inet_aton(const char *cp, struct in_addr *inp) {
    uint8_t a, b, c, d;
    if (sscanf(cp, "%hhi.%hhi.%hhi.%hhi", &a, &b, &c, &d) != 4) {
        return 0;
    }

    inp->s_addr = a | b << 8 | c << 16 | d << 24;
    return 1;
}
