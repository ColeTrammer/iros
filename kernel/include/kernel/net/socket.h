#ifndef _KERNEL_NET_SOCKET_H
#define _KERNEL_NET_SOCKET_H 1

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <kernel/fs/file.h>
#include <kernel/util/list.h>
#include <kernel/util/mutex.h>

enum socket_state { UNBOUND = 0, BOUND, LISTENING, CONNECTED, CLOSING, CLOSED };

struct socket;

struct socket_ops {
    int (*accept)(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen, int flags);
    int (*bind)(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*close)(struct socket *socket);
    int (*connect)(struct socket *socket, const struct sockaddr *addr, socklen_t addrlen);
    int (*getpeername)(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*listen)(struct socket *socket, int backlog);
    ssize_t (*sendto)(struct socket *socket, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);
    ssize_t (*recvfrom)(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen);
};

struct socket_protocol {
    int domain;
    int type;
    int protocol;
    bool is_default_protocol;
    const char *name;
    int (*create_socket)(int domain, int type, int protocol);
    struct list_node list;
};

struct socket_connection {
    union {
        struct sockaddr_un un;
        struct sockaddr_in in;
    } addr;
    socklen_t addrlen;
    unsigned long connect_to_id;
    uint32_t ack_num;
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

    struct timeval timeout;

    struct socket_connection **pending;
    int pending_length;
    int num_pending;

    bool readable : 1;
    bool writable : 1;
    bool exceptional : 1;
    bool has_peer_address : 1;
    bool has_host_address : 1;

    struct sockaddr_storage peer_address;
    struct sockaddr_storage host_address;

    mutex_t lock;

    struct socket_data *data_head;
    struct socket_data *data_tail;

    struct socket_ops *op;
    void *private_data;
};

struct socket_file_data {
    unsigned long socket_id;
};

struct socket *net_create_socket(int domain, int type, int protocol, struct socket_ops *op, int *fd, void *private_data);
ssize_t net_generic_recieve_from(struct socket *socket, void *buf, size_t len, struct sockaddr *addr, socklen_t *addrlen);
int net_generic_listen(struct socket *socket, int backlog);
int net_get_next_connection(struct socket *socket, struct socket_connection *connection);
struct socket *net_get_socket_by_id(unsigned long id);
void net_for_each_socket(void (*f)(struct socket *socket, void *data), void *data);
ssize_t net_send_to_socket(struct socket *to_send, struct socket_data *socket_data);

void net_set_host_address(struct socket *socket, const void *addr, socklen_t addrlen);
void net_set_peer_address(struct socket *socket, const void *addr, socklen_t addrlen);
void net_copy_sockaddr_to_user(const void *addr, size_t addrlen, void *user_addr, socklen_t *user_addrlen);

int net_accept(struct file *file, struct sockaddr *addr, socklen_t *addrlen, int flags);
int net_bind(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_connect(struct file *file, const struct sockaddr *addr, socklen_t addrlen);
int net_listen(struct file *file, int backlog);
int net_setsockopt(struct file *file, int level, int optname, const void *optval, socklen_t optlen);
int net_socket(int domain, int type, int protocol);
int net_getpeername(struct file *file, struct sockaddr *addr, socklen_t *addrlen);

ssize_t net_sendto(struct file *file, const void *buf, size_t len, int flags, const struct sockaddr *dest, socklen_t addrlen);
ssize_t net_recvfrom(struct file *file, void *buf, size_t len, int flags, struct sockaddr *source, socklen_t *addrlen);

void net_register_protocol(struct socket_protocol *protocol);

void init_net_sockets();

#endif /* _KERNEL_NET_SOCKET_H */
