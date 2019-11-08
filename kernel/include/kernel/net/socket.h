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
    unsigned long connect_to_id;
};

struct socket_data {
    struct socket_data *next;
    struct socket_data *prev;

    struct socket_connection from;

    size_t len;
    uint8_t data[0];
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

    struct socket_data *data_head;
    struct socket_data *data_tail;

    void *private_data;
};

struct socket_file_data {
    unsigned long socket_id;
};

struct socket *net_create_socket(int domain, int type, int protocol, int *fd);
ssize_t net_generic_recieve_from(struct socket *socket, void *buf, size_t len, struct sockaddr *addr, socklen_t *addrlen);
int net_get_next_connection(struct socket *socket, struct socket_connection *connection);
struct socket *net_get_socket_by_id(unsigned long id);
void net_for_each_socket(void (*f)(struct socket *socket, void *data), void *data);
ssize_t net_send_to_socket(struct socket *to_send, struct socket_data *socket_data);

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen);
int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_listen(struct file *file, int backlog);
int net_socket(int domain, int type, int protocol);

ssize_t net_sendto(struct file *file, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);
ssize_t net_recvfrom(struct file *file, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen);

void init_net_sockets();

#endif /* _KERNEL_NET_SOCKET_H */