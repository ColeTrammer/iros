#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/message.h>

#include "server.h"

Server::Server()
{
    m_socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    assert(m_socket_fd != -1);

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.window_server.socket");
    assert(bind(m_socket_fd, (const sockaddr*) &addr, sizeof(sockaddr_un)) == 0);
}

void Server::start()
{
    assert(listen(m_socket_fd, 10) == 0);

    sockaddr_un client_addr;
    socklen_t client_addr_len = sizeof(sockaddr_un);

    for (;;) {
        uint8_t buffer[4096];
        m_clients.for_each_reverse([&](int client_fd) {
            ssize_t data_len = read(client_fd, buffer, 4096);
            if ((data_len == -1 && errno != EAGAIN) || data_len == 0) {
                fprintf(stderr, "Closing connection\n");
                close(client_fd);
                m_clients.remove_element(client_fd);
                return;
            } else if (data_len == -1) {
                return;
            }

            fprintf(stderr, "Replying to client message\n");
        });

        int client_fd = accept4(m_socket_fd, (sockaddr*) &client_addr, &client_addr_len, SOCK_NONBLOCK);
        if (client_fd == -1 && errno != EAGAIN) {
            fprintf(stderr, "Accept failed: errno = %d\n", errno);
            assert(false);
        } else if (client_fd == -1) {
            continue;
        }

        m_clients.add(client_fd);
        WindowServerMessage message(WindowServerMessage::Type::Begin, "Hello\n");
        fprintf(stderr, "%s\n", message.message());
        write(client_fd, &message, sizeof(WindowServerMessage));
    }
}

Server::~Server()
{
    close(m_socket_fd);
}