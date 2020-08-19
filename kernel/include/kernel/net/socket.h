#ifndef _KERNEL_NET_SOCKET_H
#define _KERNEL_NET_SOCKET_H 1

#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>

#include <kernel/fs/file.h>
#include <kernel/util/hash_map.h>
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
    int (*getsockname)(struct socket *socket, struct sockaddr *addr, socklen_t *addrlen);
    int (*getsockopt)(struct socket *socket, int level, int opt, void *optval, socklen_t *optlen);
    int (*setsockopt)(struct socket *socket, int level, int opt, const void *optval, socklen_t optlen);
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
    int (*create_socket_pair)(int domain, int type, int protocol, int *fds);
    struct list_node list;
};

struct socket_connection {
    union {
        struct sockaddr_un un;
        struct sockaddr_in in;
    } addr;
    socklen_t addrlen;
    struct socket *connect_to;
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

    struct list_node socket_list;

    struct socket_connection **pending;
    int pending_length;
    int num_pending;

    bool readable : 1;
    bool writable : 1;
    bool exceptional : 1;
    bool has_peer_address : 1;
    bool has_host_address : 1;
    bool broadcast : 1;
    bool debug : 1;
    bool dont_route : 1;
    bool keepalive : 1;
    bool oob_inline : 1;
    bool reuse_addr : 1;
    bool tcp_nodelay : 1;

    int error;
    struct linger linger;

    int recv_buffer_max;
    int recv_low_water_mark;
    int send_buffer_max;
    int send_low_water_mark;
    struct timeval recv_timeout;
    struct timeval send_timeout;

    struct sockaddr_storage peer_address;
    struct sockaddr_storage host_address;

    mutex_t lock;

    struct socket_data *data_head;
    struct socket_data *data_tail;

    struct socket_ops *op;
    void *private_data;
};

struct list_node *net_get_socket_list(void);
struct socket *net_create_socket(int domain, int type, int protocol, struct socket_ops *op, void *private_data);
void net_destroy_socket(struct socket *socket);
struct socket *net_get_socket_by_id(unsigned long id);
void net_for_each_socket(void (*f)(struct hash_entry *socket, void *data), void *data);

int net_generic_setsockopt(struct socket *socket, int level, int optname, const void *optval, socklen_t optlen);
int net_generic_getsockopt(struct socket *socket, int level, int optname, void *optval, socklen_t *optlen);
int net_generic_listen(struct socket *socket, int backlog);
ssize_t net_generic_recieve_from(struct socket *socket, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen);
struct socket_data *net_get_next_message(struct socket *socket, int *error);
int net_get_next_connection(struct socket *socket, struct socket_connection *connection);
ssize_t net_send_to_socket(struct socket *to_send, struct socket_data *socket_data);

void net_set_host_address(struct socket *socket, const void *addr, socklen_t addrlen);
void net_set_peer_address(struct socket *socket, const void *addr, socklen_t addrlen);
void net_copy_sockaddr_to_user(const void *addr, size_t addrlen, void *user_addr, socklen_t *user_addrlen);

struct list_node *net_get_protocol_list(void);
void net_register_protocol(struct socket_protocol *protocol);

void init_net_sockets();

#define net_for_each_protocol(name) list_for_each_entry(net_get_protocol_list(), name, struct socket_protocol, list)

#define net_for_each_socket(name) list_for_each_entry(net_get_socket_list(), name, struct socket, socket_list)

#define NET_READ_SOCKOPT(type, optval, optlen) \
    ({                                         \
        if ((optlen) != sizeof(type)) {        \
            return -EINVAL;                    \
        }                                      \
        *(const type *) (optval);              \
    })

#define NET_WRITE_SOCKOPT(value, type, optval, optlen) \
    ({                                                 \
        int ret = 0;                                   \
        if ((*optlen) < sizeof(type)) {                \
            ret = -EINVAL;                             \
        } else {                                       \
            *(type *) (optval) = (value);              \
            *(optlen) = sizeof(type);                  \
        }                                              \
        ret;                                           \
    })

#endif /* _KERNEL_NET_SOCKET_H */
