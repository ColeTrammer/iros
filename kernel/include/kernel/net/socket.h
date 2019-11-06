#ifndef _KERNEL_NET_SOCKET_H
#define _KERNEL_NET_SOCKET_H 1

struct socket {
    int domain;
    int type;
    int protocol;

    unsigned long id;
};

struct socket_file_data {
    unsigned long socket_id;
};

struct socket *net_create_socket(int domain, int type, int protocol, int *fd);

int net_socket(int domain, int type, int protocol);

void init_net_sockets();

#endif /* _KERNEL_NET_SOCKET_H */