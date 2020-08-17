#include <arpa/inet.h>
#include <dns_service/message.h>
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

    char buffer[2048] = { 0 };
    for (;;) {
        struct sockaddr_un addr = { 0 };
        socklen_t addrlen = sizeof(struct sockaddr_un);
        int client_fd = accept(fd, (struct sockaddr *) &addr, &addrlen);
        if (client_fd == -1) {
            syslog(LOG_ERR, "Accept failed: %m");
            return 1;
        }

        ssize_t ret = read(client_fd, &buffer, 2048);
        if (ret == -1) {
            syslog(LOG_ERR, "Read from client failed: %m");
            return 1;
        } else if (ret == 0) {
            continue;
        }

        struct dns_request *request = (struct dns_request *) buffer;

        if (request->type == DNS_REQUEST_LOOKUP) {
            char *host = (char *) request->request;
            syslog(LOG_INFO, "Looking up: %s", host);

            struct host_mapping *mapping = NULL;
            mapping = lookup_host_in_mapping(known_hosts, host);
            bool was_known = !!mapping;

            // try and see if the url is an ip address
            struct in_addr a;
            if (inet_aton(host, &a) == 1) {
                mapping = calloc(1, sizeof(struct host_mapping));
                mapping->name = strdup(host);
                mapping->ip = a;
            }

            if (mapping == NULL) {
                mapping = lookup_host(host);
            }

            char response_buffer[5];
            struct dns_response *response = (struct dns_response *) response_buffer;

            if (mapping == NULL) {
                response->success = 0;
                syslog(LOG_INFO, "Mapping failed");
            } else {
                response->success = 1;
                *((in_addr_t *) response->response) = mapping->ip.s_addr;
                syslog(LOG_INFO, "Mapping succeeded: %s", inet_ntoa(mapping->ip));
            }
            write(client_fd, response_buffer, sizeof(response_buffer));

            if (!was_known) {
                free(mapping->name);
                free(mapping);
            }
        } else if (request->type == DNS_REQUEST_REVERSE) {
            in_addr_t addr = *(in_addr_t *) request->request;
            struct host_mapping *result = NULL;
            for (struct host_mapping *mapping = known_hosts; mapping; mapping = mapping->next) {
                if (mapping->ip.s_addr == addr) {
                    result = mapping;
                    break;
                }
            }

            bool was_known = !!result;

            if (!result) {
                result = lookup_address(ntohl(addr));
            }

            size_t name_len = result ? strlen(result->name) + 1 : 0;
            char response_buffer[sizeof(struct dns_response) + name_len];
            struct dns_response *response = (struct dns_response *) response_buffer;
            if (!result) {
                response->success = 0;
            } else {
                response->success = 1;
                memcpy(response->response, result->name, name_len);
            }
            write(client_fd, response_buffer, sizeof(response_buffer));

            if (!was_known) {
                free(result->name);
                free(result);
            }
        }
        close(client_fd);
    }
}
