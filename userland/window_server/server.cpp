#include <assert.h>
#include <errno.h>
#include <graphics/pixel_buffer.h>
#include <graphics/renderer.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/message.h>

#include "server.h"
#include "window.h"
#include "window_manager.h"

extern SharedPtr<PixelBuffer> g_pixels;

Server::Server(SharedPtr<PixelBuffer> pixels)
    : m_pixels(pixels)
    , m_manager(new WindowManager)
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
        auto render_window = [&](SharedPtr<Window>& window) {
            for (int x = window->rect().x(); x < window->rect().x() + window->rect().width(); x++) {
                for (int y = window->rect().y(); y < window->rect().y() + window->rect().height(); y++) {
                    m_pixels->put_pixel(x, y, window->buffer()->get_pixel(x - window->rect().x(), y - window->rect().y()));
                }
            }
        };

        assert(m_manager);
        if (m_manager->windows().size() > 0) {
            m_manager->for_each_window(render_window);
        }

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

            WindowServerMessage* message = reinterpret_cast<WindowServerMessage*>(buffer);
            switch (message->type()) {
            case WindowServerMessage::Type::Begin:
                fprintf(stderr, "Connection began\n");
                break;
            case WindowServerMessage::Type::CreatedWindow:
                fprintf(stderr, "Client is trying to tell us they created a window\n");
                break;
            case WindowServerMessage::Type::CreateWindow: {
                fprintf(stderr, "Creating window\n");
                char s[50];
                s[49] = '\0';
                snprintf(s, 49, "/window_server_%d", client_fd);

                WindowServerMessage::CreateWindowData* data = reinterpret_cast<WindowServerMessage::CreateWindowData*>(message->data());
                auto window = Window::from_shm_and_rect(s, Rect(data->x, data->y, data->width, data->height));
                m_manager->add_window(window);

                uint8_t send_buffer[4096];
                WindowServerMessage* to_send = reinterpret_cast<WindowServerMessage*>(send_buffer);
                to_send->set_data_len(strlen(s));
                to_send->set_type(WindowServerMessage::Type::CreatedWindow);
                strcpy(reinterpret_cast<char*>(to_send->data()), s);

                assert(write(client_fd, to_send, sizeof(WindowServerMessage) + to_send->data_len()) != -1);
                break;
            }
            case WindowServerMessage::Type::Invalid:
                fprintf(stderr, "Recieved invalid window server message\n");
                break;
            default:
                fprintf(stderr, "This shouldn't even be possible\n");
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

        fprintf(stderr, "Adding client: [ %d ]\n", client_fd);
        m_clients.add(client_fd);
    }
}

Server::~Server()
{
    close(m_socket_fd);
}