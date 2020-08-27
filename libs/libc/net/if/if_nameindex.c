#include <errno.h>
#include <net/if.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <unistd.h>

struct if_nameindex *if_nameindex(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return NULL;
    }

    int errno_save = errno;
    size_t i = 0;
    size_t max = 10;
    struct if_nameindex *nameindex = malloc(max * sizeof(struct if_nameindex));

    // NOTE: Probing the interface's by index sequentially won't work if for some reason the indexes aren't contiguous.
    for (;;) {
        struct ifreq req = { .ifr_ifindex = i + 1 };
        if (ioctl(fd, SIOCGIFNAME, &req) < 0) {
            if (errno != ENXIO) {
                goto error;
            }
            break;
        }

        char *new_name = strdup(req.ifr_name);
        if (!new_name) {
            goto error;
        }

        if (i + 1 < max) {
            max *= 2;
            nameindex = realloc(nameindex, max * sizeof(struct if_nameindex));
            if (!nameindex) {
                goto error;
            }
        }

        nameindex[i].if_name = new_name;
        nameindex[i].if_index = i + 1;
        i++;
    }

    if (close(fd) < 0) {
        goto error;
    }

    nameindex[i].if_name = NULL;
    nameindex[i].if_index = 0;
    errno = errno_save;
    return nameindex;

error:
    nameindex[i].if_name = NULL;
    nameindex[i].if_index = 0;
    if (nameindex) {
        if_freenameindex(nameindex);
    }
    close(fd);
    return NULL;
}
