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

#include "dns.h"
#include "mapping.h"
#include "server.h"

static void print_usage(char **argv);

int main(int argc, char **argv) {
    if (argc != 2) {
        print_usage(argv);
        return 0;
    }

    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                return start_server();
            default:
                print_usage(argv);
                return 0;
        }
    }

    char *host = argv[optind];
    fprintf(stderr, "Looking up %s\n", host);

    // try and see if the url is an ip address
    struct in_addr a;
    if (inet_aton(host, &a) == 1) {
        printf("%s\n", host);
        return 0;
    }

    struct host_mapping *known_hosts = get_known_hosts();
    struct host_mapping *local_mapping = lookup_host_in_mapping(known_hosts, host);
    if (local_mapping) {
        printf("%s\n", inet_ntoa(local_mapping->ip));
        return 0;
    }

    struct host_mapping *result = lookup_host(host);
    if (result == NULL) {
        printf("Cannot determine ip\n");
        return 1;
    }

    printf("%s\n", inet_ntoa(result->ip));
    return 0;
}

void print_usage(char **argv) {
    fprintf(stderr, "Usage: %s [-s] <host>\n", argv[0]);
}
