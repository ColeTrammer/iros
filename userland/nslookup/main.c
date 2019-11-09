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
#include <unistd.h>

struct udp_header {
    uint16_t id;

    uint8_t recursion_desired : 1;
    uint8_t truncated : 1;
    uint8_t autoratative : 1;
    uint8_t op_code : 4;
    uint8_t qr : 1;
    uint8_t response_code : 4;
    uint8_t zero : 3;
    uint8_t recursion_available : 1;
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
    return known_hosts;
}

static int num_digits(size_t s);
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
    size_t new_len = 0;

    char *part = strtok(host, ".");
    while (part != NULL) {
        size_t part_len = strlen(part);
        new_len += part_len + num_digits(part_len);

        part = strtok(NULL, ".");
    }

    char *name = malloc(new_len);
    size_t name_offset = 0;
    for (size_t i = 0; i < old_len; i++) {
        char *part = host + i;
        size_t part_len = strlen(part);
        name_offset += sprintf(name + name_offset, "%lu%s", part_len, part);
        i += part_len;
    }

    puts(name);

    return 1;
}

int num_digits(size_t s) {
    if (s == 0) {
        return 1;
    }

    int digits = 0;
    while (s > 0) {
        digits++;
        s /= 10;
    }

    return digits;
}

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s [-s] <host>", argv[0]);
}