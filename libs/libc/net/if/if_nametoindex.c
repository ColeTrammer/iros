#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

unsigned int if_nametoindex(const char *ifname) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return 0;
    }

    struct ifreq req;
    strcpy(req.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFINDEX, &req) < 0) {
        close(fd);
        return 0;
    }

    if (close(fd) < 0) {
        return 0;
    }

    return req.ifr_ifindex;
}
