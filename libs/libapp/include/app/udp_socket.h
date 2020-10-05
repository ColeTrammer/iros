#pragma once

#include <app/selectable.h>
#include <liim/string.h>
#include <netinet/in.h>

namespace App {

class UdpSocket : public Selectable {
    APP_OBJECT(UdpSocket)

public:
    UdpSocket();
    virtual ~UdpSocket() override;

    bool enable_broadcast();
    bool enable_reuse_addr();
    bool bind_to_device(const String& ifname);
    bool bind(const sockaddr_in& addr);

    ssize_t sendto(const uint8_t* buffer, size_t length, const sockaddr_in& dest);
    ssize_t recvfrom(uint8_t* buffer, size_t length, sockaddr_in* addr_out);
};

}
