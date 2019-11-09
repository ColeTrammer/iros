#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "dns.h"
#include "mapping.h"

int start_server() {
    struct host_mapping *known_hosts = get_known_hosts();

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.nslookup.socket");

    unlink(addr.sun_path);
    if (bind(fd, (const struct sockaddr*) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(fd, 5) == -1) {
        perror("listen");
        return 1;
    }

    for (;;) {
        struct sockaddr_un addr = { 0 };
        socklen_t addrlen = sizeof(struct sockaddr_un);
        int client_fd = accept(fd, (struct sockaddr*) &addr, &addrlen);
        if (client_fd == -1) {
            perror("accept");
            return 1;
        }

        char buf[2048] = { 0 };
        ssize_t ret = read(client_fd, &buf, 2048);
        if (ret == -1) {
            perror("read");
            return 1;
        } else if (ret == 0) {
            continue;
        }

        struct host_mapping *mapping = NULL;

        bool was_known = false;
        struct host_mapping *m = known_hosts;
        do {
            if (strcmp(buf, m->name) == 0) {
                mapping = m;
                was_known = true;
            }
        } while ((m = m->next) != known_hosts);

        if (mapping == NULL) {
            mapping = lookup_host(buf);
        }

        if (mapping == NULL) {
            write(client_fd, "FAILED", 7);
        } else {
            char *ip = inet_ntoa(mapping->ip);
            write(client_fd, ip, strlen(ip) + 1);
        }

        if (!was_known) {
            free(mapping);
        }
        close(client_fd);
    }
}