#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <search.h>
#include <stdbool.h>
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
    bool reverse_lookup = false;

    int opt;
    while ((opt = getopt(argc, argv, ":dr")) != -1) {
        switch (opt) {
            case 'd':
                return start_server();
            case 'r':
                reverse_lookup = true;
                break;
            default:
                print_usage(argv);
                return 2;
        }
    }

    if (optind + 1 != argc) {
        print_usage(argv);
        return 2;
    }

    if (reverse_lookup) {
        char *address_string = argv[optind];
        fprintf(stderr, "Looking up %s\n", address_string);
        struct in_addr addr;
        if (inet_aton(address_string, &addr) == 0) {
            printf("Not address: %s\n", address_string);
            return 1;
        }

        struct host_mapping *result = lookup_address(ntohl(addr.s_addr));
        if (!result) {
            printf("Failed to lookup: %s\n", address_string);
            return 1;
        }

        printf("Resolved to %s\n", result->name);
        return 0;
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
