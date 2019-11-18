#ifndef ARPA_INET_H
#define ARPA_INET_H 1

#include <inttypes.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint32_t htonl(uint32_t);
uint16_t htons(uint16_t);
uint32_t ntohl(uint32_t);
uint16_t ntohs(uint16_t);

in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr);

#ifdef __plusplus
}
#endif /* __cplusplus */

#endif /* ARPA_INET_H */