#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

#include "dns.h"
#include "mapping.h"

int start_server() {
    struct host_mapping *known_hosts = get_known_hosts();

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        syslog(LOG_ERR, "Failed to create socket: %m");
        return 1;
    }

    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.nslookup.socket");

    unlink(addr.sun_path);

    mode_t mask = umask(0002);
    if (bind(fd, (const struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        syslog(LOG_ERR, "Failed to bind: %m");
        return 1;
    }
    umask(mask);

    if (listen(fd, 5) == -1) {
        syslog(LOG_ERR, "Failed to start listening: %m");
        return 1;
    }

    char buf[2048] = { 0 };
    for (;;) {
        struct sockaddr_un addr = { 0 };
        socklen_t addrlen = sizeof(struct sockaddr_un);
        int client_fd = accept(fd, (struct sockaddr *) &addr, &addrlen);
        if (client_fd == -1) {
            syslog(LOG_ERR, "Accept failed: %m");
            return 1;
        }

        ssize_t ret = read(client_fd, &buf, 2048);
        if (ret == -1) {
            syslog(LOG_ERR, "Read from client failed: %m");
            return 1;
        } else if (ret == 0) {
            continue;
        }

        syslog(LOG_INFO, "Looking up: %s", buf);

        struct host_mapping *mapping = NULL;

        mapping = lookup_host_in_mapping(known_hosts, buf);
        bool was_known = !!mapping;

        // try and see if the url is an ip address
        struct in_addr a;
        if (inet_aton(buf, &a) == 1) {
            mapping = calloc(1, sizeof(struct host_mapping));
            mapping->name = strdup(buf);
            mapping->ip = a;
        }

        if (mapping == NULL) {
            mapping = lookup_host(buf);
        }

        if (mapping == NULL) {
            write(client_fd, "FAILED", 7);
            syslog(LOG_INFO, "Mapping failed");
        } else {
            char *ip = inet_ntoa(mapping->ip);
            write(client_fd, ip, strlen(ip) + 1);
            syslog(LOG_INFO, "Mapping succeeded: %s", ip);
        }

        if (!was_known) {
            free(mapping->name);
            free(mapping);
        }
        close(client_fd);
    }
}
