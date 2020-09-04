#include <app/event_loop.h>
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

constexpr time_t draw_timer_rate = 1000 / 60;

Server::Server(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer)
    : m_manager(make_unique<WindowManager>(fb, front_buffer, back_buffer)) {
    m_keyboard = App::SelectableFile::create(nullptr, "/dev/keyboard", O_RDONLY);
    assert(m_keyboard->valid());

    m_keyboard->on_readable = [this] {
        key_event event;
        while (read(m_keyboard->fd(), &event, sizeof(event)) == sizeof(event)) {
            auto* active_window = m_manager->active_window();
            if (active_window) {
                auto to_send = WindowServer::Message::KeyEventMessage::create(active_window->id(), event);
                assert(write(active_window->client_id(), to_send.get(), to_send->total_size()) ==
                       static_cast<ssize_t>(to_send->total_size()));
            }
        }
    };

    m_mouse = App::SelectableFile::create(nullptr, "/dev/mouse", O_RDONLY);
    assert(m_mouse->valid());

    m_mouse->on_readable = [this] {
        mouse_event event;
        while (read(m_mouse->fd(), &event, sizeof(event)) == sizeof(event)) {
            if (event.dx != 0 || event.dy != 0) {
                m_manager->notify_mouse_moved(event.dx, event.dy, event.scale_mode == SCALE_ABSOLUTE);
            }

            if (event.left != MOUSE_NO_CHANGE || event.right != MOUSE_NO_CHANGE) {
                m_manager->notify_mouse_pressed(event.left, event.right);
            }

            auto* active_window = m_manager->active_window();
            if (active_window && m_manager->should_send_mouse_events(*active_window)) {
                Point relative_mouse = m_manager->mouse_position_relative_to_window(*active_window);
                auto to_send = WindowServer::Message::MouseEventMessage::create(active_window->id(), relative_mouse.x(), relative_mouse.y(),
                                                                                event.scroll_state, event.left, event.right);
                if (write(active_window->client_id(), to_send.get(), to_send->total_size()) !=
                    static_cast<ssize_t>(to_send->total_size())) {
                    perror("write");
                    exit(1);
                }
            }
        }
    };

    m_socket_server = App::UnixSocketServer::create(nullptr, "/tmp/.window_server.socket");
    assert(m_socket_server->valid());

    m_socket_server->on_ready_to_accept = [this] {
        SharedPtr<App::UnixSocket> client;
        while ((client = m_socket_server->accept())) {
            m_clients.add(client);
            client->on_disconnect = [this](auto& client) {
                kill_client(client.fd());
            };
            client->on_ready_to_read = [this](auto& client) {
                int client_fd = client.fd();
                char buffer[BUFSIZ];
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
                    case WindowServer::Message::Type::ChangeWindowVisibilityRequest: {
                        handle_change_window_visibility_request(message, client_fd);
                        break;
                    }
                    case WindowServer::Message::Type::SwapBufferRequest: {
                        handle_swap_buffer_request(message, client_fd);
                        break;
                    }
                    case WindowServer::Message::Type::WindowReadyToResizeMessage: {
                        handle_window_ready_to_resize_message(message, client_fd);
                        break;
                    }
                    case WindowServer::Message::Type::WindowRenameRequest: {
                        handle_window_rename_request(message, client_fd);
                        break;
                    }
                    case WindowServer::Message::Type::Invalid:
                    default:
                        fprintf(stderr, "Recieved invalid window server message\n");
                        kill_client(client_fd);
                        break;
                }
            };
        }
    };

    m_manager->on_window_close_button_pressed = [this](auto window) {
        m_manager->remove_window(window);

        auto message = WindowServer::Message::WindowClosedEventMessage::create(window->id());
        if (write(window->client_id(), message.get(), message->total_size()) == -1) {
            kill_client(window->client_id());
            return;
        }
    };

    m_manager->on_window_resize_start = [this](auto window) {
        auto message = WindowServer::Message::WindowDidResizeMessage::create(window->id());
        if (write(window->client_id(), message.get(), message->total_size()) == -1) {
            kill_client(window->client_id());
            return;
        }
    };

    m_manager->on_rect_invaliadted = [this] {
        update_draw_timer();
    };

    m_draw_timer = App::Timer::create_single_shot_timer(
        nullptr,
        [this](int) {
            m_manager->draw();
        },
        draw_timer_rate);
}

void Server::kill_client(int client_id) {
    auto* client = m_clients.first_match([client_id](auto& client) {
        return client->fd() == client_id;
    });
    assert(client);
    m_manager->remove_windows_of_client(client_id);
    m_socket_server->remove_child(*client);
    m_clients.remove_element(*client);
}

void Server::update_draw_timer() {
    if (!m_manager->drawing() && m_draw_timer->expired()) {
        m_draw_timer->set_timeout(draw_timer_rate);
    }
}

void Server::handle_create_window_request(const WindowServer::Message& request, int client_fd) {
    const WindowServer::Message::CreateWindowRequest& data = request.data.create_window_request;
    SharedPtr<Window> parent;
    if (data.parent_id) {
        parent = m_manager->find_by_wid(data.parent_id);
        if (!parent) {
            kill_client(client_fd);
            return;
        }
    }

    Rect rect(data.x, data.y, data.width, data.height);
    if (parent) {
        rect.set_x(rect.x() + parent->content_rect().x());
        rect.set_y(rect.y() + parent->content_rect().y());
    }

    auto window = make_shared<Window>(rect, String(data.name), client_fd, data.type);
    if (parent) {
        Window::set_parent(window, parent);
    }

    m_manager->add_window(window);

    auto to_send =
        WindowServer::Message::CreateWindowResponse::create(window->id(), window->buffer()->size_in_bytes(), window->shm_path().string());
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_remove_window_request(const WindowServer::Message& request, int client_fd) {
    wid_t wid = request.data.remove_window_request.wid;
    auto window = m_manager->find_by_wid(wid);
    if (window) {
        kill_client(client_fd);
        return;
    }

    m_manager->remove_window(window);

    auto to_send = WindowServer::Message::RemoveWindowResponse::create(true);
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_change_window_visibility_request(const WindowServer::Message& request, int client_fd) {
    auto& data = request.data.change_window_visibility_request;
    wid_t wid = data.wid;
    auto window = m_manager->find_by_wid(wid);
    if (!window) {
        kill_client(client_fd);
        return;
    }

    bool visible = data.visible;
    if (visible) {
        window->set_position_relative_to_parent(data.x, data.y);
    }
    m_manager->set_window_visibility(window, visible);

    auto to_send = WindowServer::Message::ChangeWindowVisibilityResponse::create(wid, visible);
    assert(write(client_fd, to_send.get(), to_send->total_size()) != -1);
}

void Server::handle_swap_buffer_request(const WindowServer::Message& request, int client_id) {
    (void) client_id;

    const WindowServer::Message::SwapBufferRequest& data = request.data.swap_buffer_request;
    auto window = m_manager->find_by_wid(data.wid);
    if (!window) {
        kill_client(client_id);
        return;
    }

    window->swap();
}

void Server::handle_window_ready_to_resize_message(const WindowServer::Message& message, int client_id) {
    wid_t wid = message.data.window_ready_to_resize_message.wid;
    auto window_ptr = m_manager->find_by_wid(wid);
    if (!window_ptr) {
        kill_client(client_id);
        return;
    }

    auto& window = *window_ptr;
    if (!window.in_resize()) {
        kill_client(client_id);
        return;
    }

    auto dw = window.resize_rect().width() - window.rect().width();
    auto dh = window.resize_rect().height() - window.rect().height();
    window.relative_resize(dw, dh);
    window.set_position(window.resize_rect().x(), window.resize_rect().y());
    window.set_in_resize(false);

    auto response =
        WindowServer::Message::WindowReadyToResizeResponse::create(wid, window.content_rect().width(), window.content_rect().height());
    if (write(client_id, response.get(), response->total_size()) == -1) {
        kill_client(client_id);
    }
}

void Server::handle_window_rename_request(const WindowServer::Message& request, int client_fd) {
    const WindowServer::Message::WindowRenameRequest& data = request.data.window_rename_request;
    auto window = m_manager->find_by_wid(data.wid);
    if (!window) {
        kill_client(client_fd);
        return;
    }
    window->set_title(String(data.name));
}

void Server::start() {
    char* synchronize = getenv("__SYNCHRONIZE");
    if (synchronize) {
        int fd = atoi(synchronize);
        char c = '\0';
        write(fd, &c, 1);
        close(fd);
    }

    App::EventLoop loop;
    loop.enter();
}

Server::~Server() {}
