#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct ping_message {
    struct icmphdr header;
    char message[64 - sizeof(struct icmphdr)];
};

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s <ip>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv);
        return 0;
    }

    uint8_t a, b, c, d;
    if (sscanf(argv[1], "%hhu.%hhu.%hhu.%hhu", &a, &b, &c, &d) != 4) {
        print_usage(argv);
        return 1;
    }

    printf("ip: %u, %u, %u, %u\n", a, b, c, d);

    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = a | b << 8 | c << 16 | d << 24;

    char *message = "Ping message";

    struct ping_message ping_message = { 0 };
    ping_message.header.type = ICMP_ECHO;
    ping_message.header.code = 0;
    ping_message.header.un.echo.id = htons(getpid() & 0xFFFF);
    ping_message.header.un.echo.sequence = htons(1);

    strcpy(ping_message.message, message);

    ping_message.header.checksum = htons(in_compute_checksum(&ping_message, sizeof(struct ping_message)));

    if (sendto(fd, &ping_message, sizeof(struct ping_message), 0, (const struct sockaddr*) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("sendto");
        return 1;
    }

    for (;;) {
        socklen_t addr_size = sizeof(struct sockaddr_in);
        struct ping_message recieved_message = { 0 };
        ssize_t ret = recvfrom(fd, &recieved_message, sizeof(struct ping_message), 0, (struct sockaddr*) &addr, &addr_size);
        if (ret < 0) {
            perror("recvfrom");
            return 1;
        } else if (ret < (ssize_t) sizeof(struct icmphdr)) {
            fprintf(stderr, "Invalid response");
            return 1;
        } else {
            fprintf(stderr, "Len: %ld\n", ret);
            fprintf(stderr, "PID: %u\n", getpid() & 0xFFFF);
        }

        printf("Recived response: %u %u %u %s\n", 
            ntohs(recieved_message.header.type), ntohs(recieved_message.header.un.echo.id), 
            ntohs(recieved_message.header.un.echo.sequence), recieved_message.message);

        break;
    }

    return 0;
}