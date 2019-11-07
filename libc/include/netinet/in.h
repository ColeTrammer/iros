#ifndef _NETINET_IN_H
#define _NETINET_IN_H  1

#include <stdint.h>
#include <sys/socket.h>

#define IPPROTO_IP   0
#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17
#define IPPROTO_IPV6 41

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    sa_family_t sin_family;
    in_port_t sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

#ifdef _OS_2_SOURCE
uint16_t in_compute_checksum(void *data, size_t bytes);
#endif /* _OS_2_SOURCE */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define INADDR_NONE  ((in_addr_t) -1)

#endif /* _NETINET_IN_H */