#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <window_server/connection.h>
#include <window_server/message.h>
#include <window_server/window.h>

namespace WindowServer {

Connection::Connection() : m_fd(socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) {
    assert(m_fd != -1);

    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, "/tmp/.window_server.socket");

    assert(connect(m_fd, (sockaddr*) &addr, sizeof(sockaddr_un)) == 0);
}

Connection::~Connection() {
    close(m_fd);
}

SharedPtr<Window> Connection::create_window(int x, int y, int width, int height, const String& name, bool has_alpha, WindowType type,
                                            wid_t parent_id) {
    auto create_message = WindowServer::Message::CreateWindowRequest::create(x, y, width, height, name, type, parent_id, has_alpha);
    assert(write(m_fd, create_message.get(), create_message->total_size()) != -1);

    for (;;) {
        read_from_server();
        auto* response = m_messages.first_match([&](auto& message) {
            return message->type == Message::Type::CreateWindowResponse;
        });

        if (response) {
            Message::CreateWindowResponse& created_data = (*response)->data.create_window_response;
            auto ret = Window::construct(Rect(x, y, width, height), has_alpha, created_data, *this);
            m_messages.remove_element(*response);
            return ret;
        }
    }
}

UniquePtr<Message> Connection::recieve_message() {
    if (m_messages.empty()) {
        return nullptr;
    }

    auto ret = move(m_messages.first());
    m_messages.remove(0);
    return ret;
}

void Connection::read_from_server() {
    uint8_t buffer[0x4000];
    ssize_t ret = read(m_fd, buffer, sizeof(buffer));
    if (ret < 0) {
        if (errno == EAGAIN) {
            return;
        }
        perror("read");
        assert(false);
    }

    ssize_t offset = 0;
    while (offset < ret) {
        auto* message = reinterpret_cast<Message*>(buffer + offset);
        Message* copy = (Message*) malloc(message->total_size());
        memcpy(copy, message, message->total_size());
        m_messages.add(UniquePtr<Message>(copy));
        offset += message->total_size();
    }
}

UniquePtr<Message> Connection::send_window_ready_to_resize_message(wid_t wid) {
    auto message = WindowServer::Message::WindowReadyToResizeMessage::create(wid);
    assert(write(m_fd, message.get(), message->total_size()) != -1);

    for (;;) {
        read_from_server();

        for (int i = 0; i < m_messages.size(); i++) {
            auto& message = m_messages[i];
            if (message->type == WindowServer::Message::Type::WindowReadyToResizeResponse) {
                auto ret = move(message);
                m_messages.remove(i);
                return ret;
            }
        }
    }
}

void Connection::send_change_window_visibility_request(wid_t wid, int x, int y, bool visible) {
    auto message = WindowServer::Message::ChangeWindowVisibilityRequeset::create(wid, x, y, visible);
    assert(write(m_fd, message.get(), message->total_size()) != -1);

    for (;;) {
        read_from_server();

        for (int i = 0; i < m_messages.size(); i++) {
            auto& message = m_messages[i];
            if (message->type == WindowServer::Message::Type::ChangeWindowVisibilityResponse) {
                m_messages.remove(i);
                return;
            }
        }
    }
}

void Connection::send_remove_window_request(wid_t wid) {
    auto message = WindowServer::Message::RemoveWindowRequest::create(wid);
    assert(write(m_fd, message.get(), message->total_size()) != -1);

    for (;;) {
        read_from_server();

        for (int i = 0; i < m_messages.size(); i++) {
            auto& message = m_messages[i];
            if (message->type == WindowServer::Message::Type::RemoveWindowResponse) {
                m_messages.remove(i);
                return;
            }
        }
    }
}

void Connection::send_swap_buffer_request(wid_t wid) {
    auto swap_buffer_request = WindowServer::Message::SwapBufferRequest::create(wid);
    assert(write(m_fd, swap_buffer_request.get(), swap_buffer_request->total_size()) != -1);
}

void Connection::send_window_rename_request(wid_t wid, const String& name) {
    auto request = WindowServer::Message::WindowRenameRequest::create(wid, name);
    assert(write(m_fd, request.get(), request->total_size()) != -1);
}

}
