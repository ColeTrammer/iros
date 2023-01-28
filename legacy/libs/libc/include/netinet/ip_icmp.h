#ifndef _NETINET_IP_ICMP_H
#define _NETINET_IP_ICMP_H 1

#include <stdint.h>
#include <sys/types.h>

#define ICMP_ECHOREPLY 0
#define ICMP_ECHO      8

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct icmphdr {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union {
        struct {
            uint16_t id;
            uint16_t sequence;
        } echo;
        uint32_t gateway;
    } un;
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NETINET_IP_ICMP_H */
