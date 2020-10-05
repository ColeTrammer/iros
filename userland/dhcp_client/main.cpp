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

    auto recv_socket = App::UdpSocket::create(nullptr);
    if (!recv_socket->enable_reuse_addr()) {
        syslog(LOG_ERR, "Cannot setup dhcp recieve socket: %h");
        return 1;
    }

    sockaddr_in bind_addr = {
        .sin_family = AF_INET,
        .sin_port = DHCP_CLIENT_PORT,
        .sin_addr = { .s_addr = INADDR_ANY },
        .sin_zero = { 0 },
    };
    if (!recv_socket->bind(bind_addr)) {
        syslog(LOG_ERR, "Cannot bind dhcp recieve socket: %h");
        return 1;
    }

    int fd = socket(AF_UMESSAGE, SOCK_DGRAM | SOCK_NONBLOCK, UMESSAGE_INTERFACE);
    if (fd < 0) {
        syslog(LOG_ERR, "Cannot create umessage socket: %h");
        return 1;
    }

    umessage_interface_list_request list_req = {
        .base = { .length = sizeof(list_req), .category = UMESSAGE_INTERFACE, .type = UMESSAGE_INTERFACE_LIST_REQUEST }
    };
    if (write(fd, &list_req, sizeof(list_req)) < 0) {
        syslog(LOG_ERR, "Failed to request the interface list: %h");
        return 1;
    }

    char buffer[4096];
    ssize_t length;
    if ((length = read(fd, buffer, sizeof(buffer))) < 0) {
        syslog(LOG_ERR, "Failed to read interface list: %h");
        return 1;
    }

    HashMap<uint32_t, UniquePtr<Interface>> interfaces;

    if (!UMESSAGE_INTERFACE_LIST_VALID((umessage*) buffer, (size_t) length)) {
        syslog(LOG_ERR, "Interface list is invalid");
        return 1;
    }
    auto& list = *reinterpret_cast<umessage_interface_list*>(buffer);
    for (size_t i = 0; i < list.interface_count; i++) {
        auto& interface = list.interface_list[i];
        if ((interface.flags & IFF_RUNNING) && !(interface.flags & IFF_LOOPBACK)) {
            uint32_t xid;
            do {
                xid = (rand() << 16) | rand();
            } while (interfaces.get(xid));

            auto object = Interface::create(interface, xid);
            if (!object) {
                syslog(LOG_WARNING, "Cannot setup dhcp over interface `%s': %h", interface.name);
                continue;
            }

            if (!object->send_dhcp(DHCP_MESSAGE_TYPE_REQUEST)) {
                syslog(LOG_WARNING, "Cannot send dhcp over interface `%s': %h", interface.name);
                continue;
            }

            object->on_configured = [fd](auto& o) {
                if (write(fd, (const void*) &o.state(), o.state().base.length) < 0) {
                    syslog(LOG_WARNING, "Failed to set state for interface `%s': %h", o.name().string());
                }
            };
            interfaces.put(xid, move(object));
        }
    }

    if (interfaces.empty()) {
        return 0;
    }

    recv_socket->set_selected_events(App::NotifyWhen::Readable);
    recv_socket->enable_notifications();
    recv_socket->on_ready_to_recieve = [&] {
        dhcp_packet packet;
        while (recv_socket->recvfrom((uint8_t*) &packet, sizeof(packet)) > 0) {
            if (packet.op != DHCP_OP_REPLY) {
                syslog(LOG_INFO, "Ignoring DHCP packet with wrong DHCP op `%u'", packet.op);
                return;
            }

            auto xid = htonl(packet.xid);
            auto* interface = interfaces.get(xid);
            if (!interface) {
                syslog(LOG_INFO, "Ignoring DHCP packet with unknown xid: `%#.8X'", xid);
                return;
            }

            (*interface)->handle_dhcp_response(packet);
        }
    };

    App::EventLoop loop;
    loop.enter();
    return 0;
}
