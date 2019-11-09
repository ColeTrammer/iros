#include <arpa/inet.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mapping.h"

struct host_mapping *get_known_hosts() {
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