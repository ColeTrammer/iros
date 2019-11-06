#ifndef _KERNEL_NET_SOCKET_H
#define _KERNEL_NET_SOCKET_H 1

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <kernel/fs/file.h>
#include <kernel/util/spinlock.h>

enum socket_state {
    UNBOUND = 0,
    BOUND,
    LISTENING,
    CONNECTED
};

struct socket_connection {
    union {
        struct sockaddr_un un;
        struct sockaddr_in in;
    } addr;
    socklen_t addrlen;
};

struct socket {
    int domain;
    int type;
    int protocol;

    enum socket_state state;

    unsigned long id;

    struct socket_connection **pending;
    int pending_length;
    int num_pending;

    spinlock_t lock;

    void *private_data;
};

struct socket_file_data {
    unsigned long socket_id;
};

struct socket *net_create_socket(int domain, int type, int protocol, int *fd);

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen);
int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_listen(struct file *file, int backlog);
int net_socket(int domain, int type, int protocol);

struct socket *net_get_socket_by_id(unsigned long id);

void init_net_sockets();

#endif /* _KERNEL_NET_SOCKET_H */