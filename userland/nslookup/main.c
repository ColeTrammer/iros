#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <search.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

struct dns_header {
    uint16_t id;

    uint8_t recursion_desired : 1;
    uint8_t truncated : 1;
    uint8_t autoratative : 1;
    uint8_t op_code : 4;
    uint8_t qr : 1;
    uint8_t response_code : 4;
    uint8_t zero : 3;
    uint8_t recursion_available : 1;

    uint16_t num_questions;
    uint16_t num_answers;
    uint16_t num_records;
    uint16_t num_records_extra;
} __attribute__((packed));

struct dns_record {
    uint16_t name;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rd_length;
} __attribute__((packed));

struct host_mapping {
    struct host_mapping *next;
    struct host_mapping *prev;

    struct in_addr ip;
    char *name;
};

static struct host_mapping *get_known_hosts() {
    FILE *file = fopen("/etc/hosts", "r");
    if (!file) {
        fprintf(stderr, "Cannot open /etc/hosts\n");
        exit(1);
    }

    struct host_mapping *known_hosts = NULL;
    char *line = NULL;
    size_t line_max = 0;
    while (getline(&line, &line_max, file) != -1) {
        char name_buf[2048];
        char ip_buf[16];

        if (sscanf(line, "%2048s %16s", name_buf, ip_buf) != 2) {
            fprintf(stderr, "Invalid line in /etc/hosts: %s", line);
            exit(1);
        }

        struct host_mapping *to_add = calloc(1, sizeof(struct host_mapping));
        to_add->ip.s_addr = inet_addr(ip_buf);
        if (to_add->ip.s_addr == INADDR_NONE) {
            fprintf(stderr, "Invalid ip address in /etc/hosts: %s\n", ip_buf);
            exit(1);
        }

        to_add->name = strdup(name_buf);
        if (known_hosts == NULL) {
            known_hosts = to_add->prev = to_add->next = to_add;
        } else {
            insque(to_add, known_hosts->prev);
        }
    }

    free(line);
    fclose(file);
    return known_hosts;
}

static void print_usage(char **argv);

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv);
        return 0;
    }

    int opt;
    while ((opt = getopt(argc, argv, "s")) != -1) {
        switch (opt) {
            case 's':
                fprintf(stderr, "Server mode is not supported\n");
                return 1;
            default:
                print_usage(argv);
                return 0;
        }
    }

    char *host = argv[optind];
    fprintf(stderr, "Looking up %s\n", host);

    struct host_mapping *known_hosts = get_known_hosts();
    struct host_mapping *m = known_hosts;
    do {
        if (strcmp(host, m->name) == 0) {
            printf("%s\n", inet_ntoa(m->ip));
            return 0;
        }
    } while ((m = m->next) != known_hosts);

    size_t old_len = strlen(host);
    size_t new_len = sizeof(struct dns_header) + 5;

    char *part = strtok(host, ".");
    while (part != NULL) {
        new_len += strlen(part) + 1;
        part = strtok(NULL, ".");
    }

    uint8_t *message = calloc(new_len, sizeof(char));
    struct dns_header *header = (struct dns_header*) message;
    header->id = ntohs(getpid());
    header->qr = 0;
    header->op_code = 0;
    header->autoratative = 0;
    header->truncated = 0;
    header->recursion_desired = 1;
    header->recursion_available = 0;
    header->zero = 0;
    header->response_code = 0;
    header->num_questions = ntohs(1);
    header->num_answers = 0;
    header->num_records = 0;
    header->num_records_extra = 0;

    size_t name_offset = 0;
    for (size_t i = 0; i < old_len; i++) {
        char *part = host + i;
        size_t part_len = strlen(part);
        message[sizeof(struct dns_header) + name_offset++] = (uint8_t) part_len;
        name_offset += sprintf((char*) (message + name_offset + sizeof(struct dns_header)), "%s", part);
        i += part_len;
    }

    message[new_len - 5] = '\0';

    uint16_t *req = (uint16_t*) (message + new_len - 4);
    req[0] = htons(1);
    req[1] = htons(1);

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct timeval tv = { 1, 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(struct timeval)) == -1) {
        perror("setsockopt");
        return 1;
    }

    struct sockaddr_in dest = { 0 };
    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);
    dest.sin_addr.s_addr = inet_addr("8.8.8.8");

    if (sendto(fd, message, new_len, 0, (const struct sockaddr*) &dest, sizeof(struct sockaddr_in)) == -1) {
        perror("sendto");
        return 1;
    }

    struct sockaddr_in source = { 0 };
    socklen_t source_len = sizeof(struct sockaddr_in);

    char buf[1024];
    ssize_t read;
    if ((read = recvfrom(fd, buf, 1024, 0, (struct sockaddr*) &source, &source_len)) == -1) {
        perror("recvfrom");
        return 1;
    }

    struct dns_header *response_header = (struct dns_header*) buf;
    assert(response_header->qr == 1);

    fprintf(stderr, "Recieved response: %u, %u, %u, %u, %u\n", ntohs(response_header->id), ntohs(response_header->num_questions), ntohs(response_header->num_answers), ntohs(response_header->num_records), ntohs(response_header->num_records_extra));

    puts((char*) (response_header + 1));

    struct dns_record *record = (struct dns_record*) (buf + new_len);
    fprintf(stderr, "Received record: %u, %u, %u, %u\n", ntohs(record->type), ntohs(record->class), ntohl(record->ttl), ntohs(record->rd_length));

    struct in_addr res = { 0 };
    res.s_addr = *((uint32_t*) (record + 1));
    printf("%s\n", inet_ntoa(res));

    close(fd);

    return 1;
}

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s [-s] <host>\n", argv[0]);
}