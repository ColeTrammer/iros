#include <assert.h>
#include <ipc/endpoint.h>
#include <ipc/message_dispatcher.h>
#include <ipc/stream.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace IPC {

Endpoint::~Endpoint() {}

void Endpoint::set_socket(SharedPtr<App::UnixSocket> socket) {
    m_socket = move(socket);
    if (m_socket) {
        m_socket->on_ready_to_read = [this](auto&) {
            read_from_socket();
            handle_messages();
        };
    }
}

void Endpoint::read_from_socket() {
    char buffer[BUFSIZ];
    while (ssize_t ret = read(m_socket->fd(), buffer, sizeof(buffer)) > 0) {
        auto* raw_message = reinterpret_cast<Message*>(buffer);
        if (raw_message->size != static_cast<size_t>(ret)) {
            continue;
        }

        auto* message = reinterpret_cast<Message*>(malloc(raw_message->size));
        memcpy(message, raw_message, raw_message->size);
        m_messages.add(UniquePtr<Message>(message));
    }
}

UniquePtr<Message> Endpoint::wait_for_response_impl(uint32_t type) {
    for (;;) {
        for (int i = 0; i < m_messages.size(); i++) {
            if (m_messages[i]->type == type) {
                auto ret = move(m_messages[i]);
                m_messages.remove(i);
                handle_messages();
                return ret;
            }
        }
        read_from_socket();
    }
}

bool Endpoint::send_impl(const Message& message) {
    int fd = m_socket ? m_socket->fd() : -1;
    return write(fd, &message, message.size) == message.size;
}

void Endpoint::handle_messages() {
    assert(m_dispatcher);

    for (auto& message : m_messages) {
        Stream stream(reinterpret_cast<char*>(message.get()), message->size);
        m_dispatcher->handle_incoming_data(*this, stream);
    }
    m_messages.clear();
}

}
