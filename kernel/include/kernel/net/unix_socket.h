#ifndef _KERNEL_NET_UNIX_SOCKET_H
#define _KERNEL_NET_UNIX_SOCKET_H 1

#include <kernel/net/socket.h>

struct unix_socket_data {
    int connected_id;
};

void init_unix_sockets(void);

#endif /* _KERNEL_NET_UNIX_SOCKET_H */
