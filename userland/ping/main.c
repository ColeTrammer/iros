#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

struct ping_message {
    struct icmphdr header;
    char message[64 - sizeof(struct icmphdr)];
};

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <ip>\n", argv[0]);
}

static void on_int(int signum) {
    assert(signum == SIGINT);

    fprintf(stderr, "Done.\n");

    _exit(127 + signum);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv);
        return 0;
    }

    signal(SIGINT, &on_int);

    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct timeval tv = { 1, 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) == -1) {
        perror("setsockopt");
        return 1;
    }

    struct addrinfo *info = NULL;
    {
        int ret;
        if ((ret = getaddrinfo(argv[1], NULL, NULL, &info)) != 0) {
            fprintf(stderr, "failed to resolve host: %s (%d)\n", argv[1], ret);
            return 1;
        }
    }

    struct sockaddr_in addr;
    memcpy(&addr, info->ai_addr, sizeof(struct sockaddr_in));

    freeaddrinfo(info);

    if (addr.sin_addr.s_addr == INADDR_NONE) {
        print_usage(argv);
        return 1;
    }

    fprintf(stderr, "PING: %s\n", inet_ntoa(addr.sin_addr));

    char *message = "Ping message";

    struct ping_message ping_message = { 0 };
    ping_message.header.type = ICMP_ECHO;
    ping_message.header.code = 0;
    ping_message.header.un.echo.id = htons(getpid() & 0xFFFF);

    int sequence = 1;

    strcpy(ping_message.message, message);

    for (;;) {
        ping_message.header.checksum = 0;
        ping_message.header.un.echo.sequence = htons(sequence++);
        ping_message.header.checksum = htons(in_compute_checksum(&ping_message, sizeof(struct ping_message)));
        if (sendto(fd, &ping_message, sizeof(struct ping_message), 0, (const struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
            perror("sendto");
            return 1;
        }

        socklen_t addr_size = sizeof(struct sockaddr_in);
        struct ping_message recieved_message = { 0 };
        ssize_t ret = recvfrom(fd, &recieved_message, sizeof(struct ping_message), 0, (struct sockaddr *) &addr, &addr_size);
        if (ret < 0) {
            if (errno = EINTR) {
                printf("Timed out: seq %d\n", sequence - 1);
                continue;
            }
            perror("recvfrom");
            return 1;
        } else if (ret < (ssize_t) sizeof(struct icmphdr)) {
            fprintf(stderr, "Invalid response");
            return 1;
        }

        printf("response from: %s: seq %d\n", inet_ntoa(addr.sin_addr), ntohs(recieved_message.header.un.echo.sequence));

        sleep(1);
    }

    return 0;
}