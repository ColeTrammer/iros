#include <arpa/inet.h>

uint16_t htons(uint16_t value) {
#if BTYE_ORDER==LITTLE_ENDIAN
    return __builtin_bswap16(value);
#endif /* BTYE_ORDER==LITTLE_ENDIAN */
    return value;
}

uint16_t ntohs(uint16_t value) {
    return htons(value);
}

uint32_t htonl(uint32_t value) {
#if BTYE_ORDER==LITTLE_ENDIAN
    return __builtin_bswap32(value);
#endif /* BTYE_ORDER==LITTLE_ENDIAN */
    return value;
}

uint32_t ntohl(uint32_t value) {
    return htonl(value);
}