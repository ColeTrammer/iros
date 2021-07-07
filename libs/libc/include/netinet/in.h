#ifndef _NETINET_IN_H
#define _NETINET_IN_H 1

#include <inttypes.h>
#include <sys/socket.h>

#define IPPROTO_IP   0
#define IPPROTO_ICMP 1
#define IPPROTO_TCP  6
#define IPPROTO_UDP  17
#define IPPROTO_IPV6 41

#define IP_ADD_MEMBERSHIP         1
#define IP_DROP_MEMBERSHIP        2
#define IP_TTL                    3
#define IP_MULTICAST_TTL          4
#define IP_MULTICAST_LOOP         5
#define IP_MULTICAST_IF           6
#define IP_ADD_SOURCE_MEMBERSHIP  7
#define IP_DROP_SOURCE_MEMBERSHIP 8

#define IPV6_JOIN_GROUP     1
#define IPV6_LEAVE_GROUP    2
#define IPV6_MULTICAST_HOPS 3
#define IPV6_MULTICAST_IF   4
#define IPV6_MULTICAST_LOOP 5
#define IPV6_UNICAST_HOPS   6
#define IPV6_V6ONLY         7

#define INET_ADDRSTRLEN  16
#define INET6_ADDRSTRLEN 46

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

struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr_in6 {
    sa_family_t sin6_family;
    in_port_t sin6_port;
    uint32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    uint32_t sin6_scope_id;
};

struct ip_mreq {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
};

struct ip_mreq_source {
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
    struct in_addr imr_sourceaddr;
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr;
    unsigned int ipv6mr_interface;
};

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define INADDR_NONE     ((in_addr_t) -1)
#define INADDR_ANY      ((in_addr_t) 0)
#define INADDR_LOOPBACK ((in_addr_t) (1 | 0 | 0 | (127 << 24)))

#define IN6ADDR_ANY_INIT                                   \
    {                                                      \
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } \
    }

#define IN6ADDR_LOOPBACK_INIT                              \
    {                                                      \
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } \
    }

#endif /* _NETINET_IN_H */
