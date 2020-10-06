#include <app/event_loop.h>
#include <app/udp_socket.h>
#include <arpa/inet.h>
#include <liim/hash_map.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/umessage.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include "dhcp.h"
#include "interface.h"

void print_usage_and_exit(const char* s) {
    fprintf(stderr, "Usage: %s\n", s);
    exit(2);
}

int main(int argc, char** argv) {
    srand(time(nullptr));

    int opt;
    while ((opt = getopt(argc, argv, ":")) != -1) {
        switch (opt) {
            case ':':
            case '?':
                print_usage_and_exit(*argv);
                break;
        }
    }

    if (optind != argc) {
        print_usage_and_exit(*argv);
    }

    int fd = socket(AF_UMESSAGE, SOCK_DGRAM | SOCK_NONBLOCK, UMESSAGE_INTERFACE);
    if (fd < 0) {
        syslog(LOG_ERR, "Cannot create umessage socket: %m");
        return 1;
    }

    umessage_interface_list_request list_req = {
        .base = { .length = sizeof(list_req), .category = UMESSAGE_INTERFACE, .type = UMESSAGE_INTERFACE_LIST_REQUEST }
    };
    if (write(fd, &list_req, sizeof(list_req)) < 0) {
        syslog(LOG_ERR, "Failed to request the interface list: %m");
        return 1;
    }

    char buffer[4096];
    ssize_t length;
    if ((length = read(fd, buffer, sizeof(buffer))) < 0) {
        syslog(LOG_ERR, "Failed to read interface list: %m");
        return 1;
    }

    HashMap<uint32_t, SharedPtr<Interface>> interfaces;

    if (!UMESSAGE_INTERFACE_LIST_VALID((umessage*) buffer, (size_t) length)) {
        syslog(LOG_ERR, "Interface list is invalid");
        return 1;
    }
    auto& list = *reinterpret_cast<umessage_interface_list*>(buffer);
    for (size_t i = 0; i < list.interface_count; i++) {
        auto& interface = list.interface_list[i];
        if (!!(interface.flags & IFF_RUNNING) && !(interface.flags & IFF_LOOPBACK)) {
            uint32_t xid;
            do {
                xid = (rand() << 16) | rand();
            } while (interfaces.get(xid));

            auto object = Interface::create(interface, xid);
            if (!object) {
                syslog(LOG_WARNING, "Cannot setup dhcp over interface `%s': %m", interface.name);
                continue;
            }

            if (!object->send_dhcp(DHCP_MESSAGE_TYPE_DISCOVER)) {
                syslog(LOG_WARNING, "Cannot send dhcp over interface `%s': %m", interface.name);
                continue;
            }

            object->on_configured = [fd](auto& o) {
                if (write(fd, (const void*) &o.state(), o.state().base.length) < 0) {
                    syslog(LOG_WARNING, "Failed to set state for interface `%s': %m", o.name().string());
                }
            };
            interfaces.put(xid, move(object));
            syslog(LOG_INFO, "Sent DHCP request for interface `%s'", interface.name);
            continue;
        }

        syslog(LOG_INFO, "Ignoring interface `%s'", interface.name);
    }

    if (interfaces.empty()) {
        return 0;
    }

    App::EventLoop loop;
    loop.enter();
    return 0;
}
