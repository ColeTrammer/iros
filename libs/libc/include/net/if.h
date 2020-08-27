#ifndef _NET_IF_H
#define _NET_IF_H 1

#include <sys/socket.h>

#define IF_NAMESIZE 16
#define IFNAMSIZ    IF_NAMESIZE

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ifreq {
    char ifr_name[IF_NAMESIZE];
    union {
        struct sockaddr ifr_addr;
        struct sockaddr ifr_dstaddr;
        struct sockaddr ifr_broadaddr;
        struct sockaddr ifr_netmask;
        struct sockaddr ifr_hwaddr;
        short ifr_flags;
        int ifr_ifindex;
        int ifr_metric;
        int ifr_mtu;
        char ifr_slave[IF_NAMESIZE];
        char ifr_newname[IF_NAMESIZE];
        char *ifr_data;
    };
};

struct if_nameindex {
    unsigned int if_index;
    char *if_name;
};

void if_freenameindex(struct if_nameindex *nameindex);
char *if_indextoname(unsigned int index, char *name);
struct if_nameindex *if_nameindex(void);
unsigned int if_nametoindex(const char *name);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NET_IF_H */
