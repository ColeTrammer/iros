#ifndef ARPA_INET_H
#define ARPA_INET_H 1

#include <netinet/in.h>
#include <stdint.h>

uint32_t htonl(uint32_t);
uint16_t htons(uint16_t);
uint32_t ntohl(uint32_t);
uint16_t ntohs(uint16_t);

in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr);

#endif /* ARPA_INET_H */