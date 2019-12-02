#include <arpa/inet.h>
#include <stdio.h>

uint16_t htons(uint16_t value) {
#if BTYE_ORDER == LITTLE_ENDIAN
    return __builtin_bswap16(value);
#endif /* BTYE_ORDER==LITTLE_ENDIAN */
    return value;
}

uint16_t ntohs(uint16_t value) {
    return htons(value);
}

uint32_t htonl(uint32_t value) {
#if BTYE_ORDER == LITTLE_ENDIAN
    return __builtin_bswap32(value);
#endif /* BTYE_ORDER==LITTLE_ENDIAN */
    return value;
}

uint32_t ntohl(uint32_t value) {
    return htonl(value);
}

in_addr_t inet_addr(const char *cp) {
    uint8_t a, b, c, d;
    if (sscanf(cp, "%hhi.%hhi.%hhi.%hhi", &a, &b, &c, &d) != 4) {
        return INADDR_NONE;
    }

    return a | b << 8 | c << 16 | d << 24;
}

static char ntoa_buf[16] = { 0 };

char *inet_ntoa(struct in_addr in) {
    snprintf(ntoa_buf, 16, "%.3d.%.3d.%.3d.%.3d", (in.s_addr & 0x000000FF) >> 0, (in.s_addr & 0x0000FF00) >> 8,
             (in.s_addr & 0x00FF0000) >> 16, (in.s_addr & 0xFF000000) >> 24);
    return ntoa_buf;
}
