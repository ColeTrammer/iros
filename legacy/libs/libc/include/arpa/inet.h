#ifndef ARPA_INET_H
#define ARPA_INET_H 1

#include <inttypes.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if BTYE_ORDER == LITTLE_ENDIAN
#define ntohs(x) __builtin_bswap16(x)
#define htons(x) __builtin_bswap16(x)
#define ntohl(x) __builtin_bswap32(x)
#define htonl(x) __builtin_bswap32(x)
#else
#define ntohs(x) x
#define htons(x) x
#define ntohl(x) x
#define htonl(x) x
#endif /* BTYE_ORDER==LITTLE_ENDIAN */

in_addr_t inet_addr(const char *cp);
int inet_aton(const char *cp, struct in_addr *inp);
char *inet_ntoa(struct in_addr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ARPA_INET_H */
