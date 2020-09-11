#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static uint16_t port;
static struct in_addr in_addr;
static bool use_default_port = true;
static bool use_default_addr = true;
static bool do_send = false;
static bool tcp = false;

static int do_tcp(void) {
    if (use_default_port) {
        port = 8823;
    }

    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
        perror("vm_internet_test: socket");
        return 1;
    }

    if (do_send) {
        if (use_default_addr) {
            in_addr.s_addr = htonl(INADDR_LOOPBACK);
        }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = ntohs(port),
            .sin_addr = in_addr,
            .sin_zero = { 0 },
        };

        if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            perror("vm_internet_test: connect");
            return 1;
        }

        char buffer[4000] = "Hello, World";
        if (send(fd, buffer, sizeof(buffer), 0) != sizeof(buffer)) {
            perror("vm_internet_test: send");
            return 1;
        }

        if (close(fd) < 0) {
            perror("vm_internet_test: close");
            return 1;
        }

        return 0;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = ntohs(port),
        .sin_addr = in_addr,
        .sin_zero = { 0 },
    };

    if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
        perror("vm_internet_test: bind");
        return 1;
    }

    if (listen(fd, 5) < 0) {
        perror("vm_internet_test: listen");
        return 1;
    }

    int sfd = accept(fd, NULL, NULL);
    if (sfd < 0) {
        perror("vm_internet_test: accept");
        return 1;
    }

    char buffer[BUFSIZ];
    ssize_t bytes;
    if ((bytes = recv(sfd, buffer, sizeof(buffer), MSG_WAITALL)) < 0) {
        perror("vm_internet_test: recvfrom");
        return 1;
    }

    if (close(fd) < 0) {
        perror("vm_internet_test: close");
        return 1;
    }

    printf("[%ld]: '%s'\n", bytes, buffer);
    return 0;
}

static int do_udp(void) {
    if (use_default_port) {
        port = 8888;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("vm_internet_test: socket");
        return 1;
    }

    if (do_send) {
        if (use_default_addr) {
            in_addr.s_addr = htonl(INADDR_LOOPBACK);
        }

        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = ntohs(port),
            .sin_addr = in_addr,
            .sin_zero = { 0 },
        };

        char buffer[4000] = "Hello, World";
        if (sendto(fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(buffer)) {
            perror("vm_internet_test: sendto");
            return 1;
        }

        printf("Sent %lu bytes to %s:%u\n", sizeof(buffer), inet_ntoa(in_addr), port);
        return 0;
    }

    if (use_default_addr) {
        in_addr.s_addr = htonl(INADDR_ANY);
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = ntohs(port),
        .sin_addr = in_addr,
        .sin_zero = { 0 },
    };

    if (bind(fd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) < 0) {
        perror("vm_internet_test: bind");
        return 1;
    }

    struct timeval recv_timeout = { .tv_sec = 5, .tv_usec = 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout)) < 0) {
        perror("vm_internet_test: setsockopt(SO_RCVTIMEO)");
        return 1;
    }

    char buffer[BUFSIZ];
    ssize_t bytes;
    if ((bytes = recv(fd, buffer, sizeof(buffer), 0)) < 0) {
        perror("vm_internet_test: recvfrom");
        return 1;
    }

    printf("[%ld]: '%s'\n", bytes, buffer);
    return 0;
}

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-r|-s] [-t] [-i ip-address] [-p port]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, ":i:p:rst")) != -1) {
        switch (opt) {
            case 'i':
                if (inet_aton(optarg, &in_addr) == 0) {
                    print_usage_and_exit(*argv);
                }
                use_default_addr = false;
                break;
            case 'p':
                port = atoi(optarg);
                use_default_port = false;
                break;
            case 'r':
                do_send = false;
                break;
            case 's':
                do_send = true;
                break;
            case 't':
                tcp = true;
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (tcp) {
        return do_tcp();
    }

    return do_udp();
}
