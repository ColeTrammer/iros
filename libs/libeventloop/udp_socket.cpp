#include <errno.h>
#include <eventloop/udp_socket.h>
#include <net/if.h>
#include <sys/socket.h>
#include <unistd.h>

namespace App {

UdpSocket::UdpSocket() {
    set_fd(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
}

UdpSocket::~UdpSocket() {
    if (fd() >= 0) {
        close(fd());
    }
}

bool UdpSocket::enable_broadcast() {
    int s = 1;
    return !setsockopt(fd(), SOL_SOCKET, SO_BROADCAST, &s, sizeof(s));
}

bool UdpSocket::enable_reuse_addr() {
    int s = 1;
    return !setsockopt(fd(), SOL_SOCKET, SO_REUSEADDR, &s, sizeof(s));
}

bool UdpSocket::bind_to_device(const String& name) {
    if (name.size() >= IF_NAMESIZE) {
        errno = E2BIG;
        return false;
    }

    return !setsockopt(fd(), SOL_SOCKET, SO_BINDTODEVICE, name.string(), name.size() + 1);
}

bool UdpSocket::bind(const sockaddr_in& addr) {
    return !::bind(fd(), (sockaddr*) &addr, sizeof(addr));
}

ssize_t UdpSocket::sendto(const uint8_t* buffer, size_t length, const sockaddr_in& dest) {
    return ::sendto(fd(), buffer, length, 0, (const sockaddr*) &dest, sizeof(dest));
}

ssize_t UdpSocket::recvfrom(uint8_t* buffer, size_t length, sockaddr_in* addr_out) {
    socklen_t addrlen;
    return ::recvfrom(fd(), buffer, length, 0, (sockaddr*) addr_out, addr_out ? &addrlen : nullptr);
}

}
