#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

uint16_t net_ip_v4_compute_checksum(void *packet, size_t num_bytes) {
    uint16_t *raw_data = (uint16_t*) packet;
    uint32_t sum = 0;

    // Sum everything 16 bits at a time
    for (size_t i = 0; i < num_bytes / sizeof(uint16_t); i++) {
        // Prevent overflow
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        sum += ntohs(raw_data[i]);
    }

    // 1's complement the carry
    while (sum & ~0xFFFF) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Invert the sum for storage
    return (uint16_t) (~sum & 0xFFFF);
}

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

    struct icmphdr header = { 0 };
    header.type = ICMP_ECHO;
    header.code = 0;
    header.un.echo.id = htons(getpid());
    header.un.echo.sequence = htons(1);

    header.checksum = net_ip_v4_compute_checksum(&header, sizeof(struct icmphdr));

    if (sendto(fd, &header, sizeof(struct icmphdr), 0, (const struct sockaddr*) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("sendto");
        return 1;
    }

    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct icmphdr response = { 0 };
    recvfrom(fd, &response, sizeof(struct icmphdr), 0, (struct sockaddr*) &addr, &addr_size);

    printf("Recived response: %u %u %u\n", ntohs(response.type), ntohs(response.un.echo.id), htons(response.un.echo.sequence));

    return 0;
}