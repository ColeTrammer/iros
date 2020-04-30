#include <assert.h>
#include <errno.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <liim/pointers.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/message.h>

#include "server.h"
#include "window.h"
#include "window_manager.h"

Server::Server(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_manager(make_unique<WindowManager>(fb, front_buffer, back_buffer)) {
    m_kbd_fd = open("/dev/keyboard", O_RDONLY);
    assert(m_kbd_fd != -1);

    m_mouse_fd = open("/dev/mouse", O_RDONLY);
    assert(m_mouse_fd != -1);

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
    m_manager->remove_windows_of_client(client_id);
}

void Server::handle_create_window_request(const WindowServer::Message& request, int client_fd) {
    char s[50];
    s[49] = '\0';
    snprintf(s, 49, "/window_server_%d", 2 * client_fd);

    const WindowServer::Message::CreateWindowRequest& data = request.data.create_window_request;
    auto window = make_shared<Window>(s, Rect(data.x, data.y, data.width, data.height), client_fd);
    m_manager->add_window(window);

    auto to_send = WindowServer::Message::CreateWindowResponse::create(window->id(), window->buffer()->size_in_bytes(), s);
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_remove_window_request(const WindowServer::Message& request, int client_fd) {
    wid_t wid = request.data.remove_window_request.wid;
    m_manager->remove_window(wid);

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

    fd_set set;
    fd_set exceptional;
    uint8_t buffer[BUFSIZ];

    timespec time_of_last_paint { 0, 0 };
    bool did_draw = false;
    long remaining_time = 0;
    for (;;) {
        timespec current_time;
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        if (current_time.tv_sec - time_of_last_paint.tv_sec > 2) {
            m_manager->draw();
            did_draw = true;
        } else {
            long min_delta_time = 1000 / 60;
            long delta_time =
                (current_time.tv_sec - time_of_last_paint.tv_sec) * 1000 + (current_time.tv_nsec - time_of_last_paint.tv_nsec) / 1000000;
            if (delta_time >= min_delta_time) {
                m_manager->draw();
                did_draw = true;
            } else {
                remaining_time = delta_time - min_delta_time;
                did_draw = false;
            }
        }

        time_of_last_paint = current_time;

        FD_ZERO(&set);
        FD_ZERO(&exceptional);
        FD_SET(m_socket_fd, &set);
        FD_SET(m_kbd_fd, &set);
        FD_SET(m_mouse_fd, &set);
        m_clients.for_each([&](int fd) {
            FD_SET(fd, &set);
            FD_SET(fd, &exceptional);
        });

        timespec timeout { .tv_sec = 0, .tv_nsec = remaining_time * 1000000 };
        timespec* timeout_to_pass = did_draw ? nullptr : &timeout;
        int ret = select(FD_SETSIZE, &set, nullptr, &exceptional, timeout_to_pass);
        if (ret < 0) {
            perror("select");
            exit(1);
        }

        m_clients.for_each_reverse([&](int client_fd) {
            if (FD_ISSET(client_fd, &exceptional)) {
                // At this point, just assume the client disconnected.
                // There's probably no other way this will be set.
                kill_client(client_fd);
                return;
            }

            if (!FD_ISSET(client_fd, &set)) {
                return;
            }

            ssize_t data_len = read(client_fd, buffer, sizeof(buffer));
            if (data_len <= 0) {
                kill_client(client_fd);
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

        if (FD_ISSET(m_kbd_fd, &set)) {
            key_event event;
            while (read(m_kbd_fd, &event, sizeof(event)) == sizeof(event)) {
                auto* active_window = m_manager->active_window();
                if (active_window) {
                    auto to_send = WindowServer::Message::KeyEventMessage::create(event);
                    assert(write(active_window->client_id(), to_send.get(), to_send->total_size()) ==
                           static_cast<ssize_t>(to_send->total_size()));
                }
            }
        }

        if (FD_ISSET(m_mouse_fd, &set)) {
            mouse_event event;
            while (read(m_mouse_fd, &event, sizeof(event)) == sizeof(event)) {
                if (event.dx != 0 || event.dy != 0) {
                    m_manager->notify_mouse_moved(event.dx, event.dy, event.scale_mode == SCALE_RELATIVE);
                }

                if (event.left == MOUSE_DOWN || event.right == MOUSE_DOWN) {
                    m_manager->notify_mouse_pressed();
                }

                auto* active_window = m_manager->active_window();
                if (active_window) {
                    auto to_send = WindowServer::Message::MouseEventMessage::create(event);
                    assert(write(active_window->client_id(), to_send.get(), to_send->total_size()) ==
                           static_cast<ssize_t>(to_send->total_size()));
                }
            }
        }

        if (FD_ISSET(m_socket_fd, &set)) {
            int client_fd = accept4(m_socket_fd, (sockaddr*) &client_addr, &client_addr_len, SOCK_NONBLOCK);
            if (client_fd == -1 && errno != EAGAIN) {
                perror("accept");
                exit(1);
            } else if (client_fd == -1) {
                continue;
            }

            m_clients.add(client_fd);
        }
    }
}

Server::~Server() {
    close(m_socket_fd);
}