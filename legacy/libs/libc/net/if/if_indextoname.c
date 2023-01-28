#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

char *if_indextoname(unsigned int ifindex, char *ifname) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return NULL;
    }

    struct ifreq req = { .ifr_ifindex = ifindex };
    if (ioctl(fd, SIOCGIFNAME, &req) < 0) {
        close(fd);
        return NULL;
    }

    if (close(fd) < 0) {
        return NULL;
    }

    strcpy(ifname, req.ifr_name);
    return ifname;
}
