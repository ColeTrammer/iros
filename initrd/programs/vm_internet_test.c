#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static void print_usage_and_exit(const char *s) {
    fprintf(stderr, "Usage: %s [-h|-g] [-p port]\n", s);
    exit(2);
}

int main(int argc, char **argv) {
    int opt;

    uint16_t port = 8888;
    bool host_mode = false;
    while ((opt = getopt(argc, argv, ":hgp:")) != -1) {
        switch (opt) {
            case 'h':
                host_mode = true;
                break;
            case 'g':
                host_mode = false;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0) {
        perror("vm_internet_test: socket");
        return 1;
    }

    if (host_mode) {
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = ntohs(port),
            .sin_addr = { .s_addr = ntohl(INADDR_LOOPBACK) },
            .sin_zero = { 0 },
        };

        char buffer[500] = "Hello, World";
        if (sendto(fd, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) != sizeof(buffer)) {
            perror("vm_internet_test: sendto");
            return 1;
        }

        return 0;
    }

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = ntohs(port),
        .sin_addr = { .s_addr = htonl(INADDR_ANY) },
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
