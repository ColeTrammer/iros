#include <errno.h>
#include <net/if.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#ifdef __iros__
#include <sys/umessage.h>
#else
#include <sys/ioctl.h>
#endif /* __iros__ */

struct if_nameindex *if_nameindex(void) {
#ifdef __iros__
    int fd = socket(AF_UMESSAGE, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, UMESSAGE_INTERFACE);
#else
    int fd = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
#endif /* __iros__ */
    if (fd < 0) {
        return NULL;
    }

    int errno_save = errno;
    size_t i = 0;
    size_t max = 10;
    struct if_nameindex *nameindex = malloc(max * sizeof(struct if_nameindex));

#ifdef __iros__
    struct umessage_interface_list_request req = {
        .base = { .category = UMESSAGE_INTERFACE,
                  .type = UMESSAGE_INTERFACE_LIST_REQUEST,
                  .length = sizeof(struct umessage), 
        },
    };
    if (write(fd, &req, req.base.length) < 0) {
        goto error;
    }

    char buffer[2048];
    ssize_t length;
    if ((length = read(fd, buffer, sizeof(buffer))) < 0) {
        goto error;
    }

    struct umessage_interface_list *list = (void *) buffer;
    for (; i < list->interface_count; i++) {
        char *new_name = strdup(list->interface_list[i].name);
        int new_index = list->interface_list[i].index;
#else
    // NOTE: Probing the interface's by index sequentially won't work if for some reason the indexes aren't contiguous.
    for (;; i++) {
        struct ifreq req = { .ifr_ifindex = i + 1 };
        if (ioctl(fd, SIOCGIFNAME, &req) < 0) {
            if (errno != ENXIO) {
                goto error;
            }
            break;
        }

        char *new_name = strdup(req.ifr_name);
        int new_index = i + 1;
#endif /* __iros__ */

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
        nameindex[i].if_index = new_index;
    }

    if (close(fd) < 0) {
        fd = -1;
        goto error;
    }

    nameindex[i].if_name = NULL;
    nameindex[i].if_index = 0;
    errno = errno_save;
    return nameindex;

error:
    if (nameindex) {
        if_freenameindex(nameindex);
    }
    if (fd >= 0) {
        close(fd);
    }
    return NULL;
}
