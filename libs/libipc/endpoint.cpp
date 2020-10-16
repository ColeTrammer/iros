#include <assert.h>
#include <ipc/endpoint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace IPC {

void Endpoint::initialize() {
    m_socket = App::UnixSocket::create_connection(shared_from_this(), m_path);
    m_socket->on_ready_to_read = [this](auto&) {
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

        handle_messages();
    };
}

void Endpoint::handle_messages() {
    assert(m_dispatcher);

    for (auto& message : m_messages) {
        Stream stream(reinterpret_cast<char*>(message.get()), message->size);
        m_dispatcher->handle_incoming_data(stream);
    }
    m_messages.clear();
}

}
