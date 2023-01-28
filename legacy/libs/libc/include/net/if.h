#ifndef _NET_IF_H
#define _NET_IF_H 1

#include <sys/socket.h>

#define IF_NAMESIZE 16
#define IFNAMSIZ    IF_NAMESIZE

#define IFF_UP          1
#define IFF_BROADCAST   2
#define IFF_DEBUG       4
#define IFF_LOOPBACK    8
#define IFF_POINTOPOINT 16
#define IFF_RUNNING     32
#define IFF_NOARP       64
#define IFF_PROMISC     128
#define IFF_NOTRAILERS  256
#define IFF_ALLMULTI    512
#define IFF_MASTER      1024
#define IFF_SLAVE       2048
#define IFF_MULTICAST   4096
#define IFF_PORTSEL     8192
#define IFF_AUTOMEDIA   16384
#define IFF_DYNAMIC     32768

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
