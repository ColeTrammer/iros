#include <assert.h>
#include <errno.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/message.h>

#include "server.h"
#include "window.h"
#include "window_manager.h"

Server::Server(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_manager(make_unique<WindowManager>(fb, front_buffer, back_buffer)) {
    m_socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    assert(m_socket_fd != -1);

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.window_server.socket");
    assert(bind(m_socket_fd, (const sockaddr*) &addr, sizeof(sockaddr_un)) == 0);
}

void Server::kill_client(int client_id) {
    close(client_id);
    m_clients.remove_element(client_id);
    m_manager->windows().remove_if([&](auto window) {
        return window->client_id() == client_id;
    });
}

void Server::handle_create_window_request(const WindowServer::Message& request, int client_fd) {
    char s[50];
    s[49] = '\0';
    snprintf(s, 49, "/window_server_%d", client_fd);

    const WindowServer::Message::CreateWindowRequest& data = request.data.create_window_request;
    auto window = make_shared<Window>(s, Rect(data.x, data.y, data.width, data.height), client_fd);
    m_manager->add_window(window);

    auto to_send = WindowServer::Message::CreateWindowResponse::create(window->id(), window->buffer()->size_in_bytes(), s);
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_remove_window_request(const WindowServer::Message& request, int client_fd) {
    wid_t wid = request.data.remove_window_request.wid;
    m_manager->windows().remove_if([&](auto& window) {
        return window->id() == wid;
    });

    auto to_send = WindowServer::Message::RemoveWindowResponse::create(true);
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_swap_buffer_request(const WindowServer::Message& request, int client_id) {
    (void) client_id;

    const WindowServer::Message::SwapBufferRequest& data = request.data.swap_buffer_request;
    m_manager->windows().for_each([&](auto& window) {
        if (window->id() == data.wid) {
            window->swap();
        }
    });
}

void Server::start() {
    assert(listen(m_socket_fd, 10) == 0);

    sockaddr_un client_addr;
    socklen_t client_addr_len = sizeof(sockaddr_un);

    for (;;) {
        m_manager->draw();

        uint8_t buffer[4096];
        m_clients.for_each_reverse([&](int client_fd) {
            ssize_t data_len = read(client_fd, buffer, 4096);
            if ((data_len == -1 && errno != EAGAIN) || data_len == 0) {
                kill_client(client_fd);
                return;
            } else if (data_len == -1) {
                return;
            }

            WindowServer::Message& message = *reinterpret_cast<WindowServer::Message*>(buffer);
            switch (message.type) {
                case WindowServer::Message::Type::CreateWindowRequest: {
                    handle_create_window_request(message, client_fd);
                    break;
                }
                case WindowServer::Message::Type::RemoveWindowRequest: {
                    handle_remove_window_request(message, client_fd);
                    break;
                }
                case WindowServer::Message::Type::SwapBufferRequest: {
                    handle_swap_buffer_request(message, client_fd);
                    break;
                }
                case WindowServer::Message::Type::Invalid:
                default:
                    fprintf(stderr, "Recieved invalid window server message\n");
                    kill_client(client_fd);
                    break;
            }
        });

        int client_fd = accept4(m_socket_fd, (sockaddr*) &client_addr, &client_addr_len, SOCK_NONBLOCK);
        if (client_fd == -1 && errno != EAGAIN) {
            fprintf(stderr, "Accept failed: errno = %d\n", errno);
            assert(false);
        } else if (client_fd == -1) {
            continue;
        }

        m_clients.add(client_fd);
    }
}

Server::~Server() {
    close(m_socket_fd);
}