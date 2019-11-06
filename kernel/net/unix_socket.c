#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/net/socket.h>

int net_unix_socket(int domain, int type, int protocol) {
    if (type != SOCK_STREAM || protocol != 0) {
        return -EPROTONOSUPPORT;
    }

    int fd;
    struct socket *socket = net_create_socket(domain, type, protocol, &fd);
    (void) socket;

    return fd;
}