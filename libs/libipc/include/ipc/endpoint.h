#pragma once

#include <app/unix_socket.h>
#include <ipc/message.h>
#include <ipc/message_dispatcher.h>
#include <unistd.h>

namespace IPC {

class Endpoint : public App::Object {
public:
    Endpoint(String path) : m_path(move(path)) {}
    virtual void initialize() override;

    void set_dispatcher(SharedPtr<MessageDispatcher> dispatcher) { m_dispatcher = move(dispatcher); }

    template<ConcreteMessage T>
    bool send(const T& val) {
        char buffer[4096];
        if (val.serialization_size() > sizeof(buffer)) {
            return false;
        }
        Stream stream(buffer, sizeof(buffer));
        if (!val.serialize(stream)) {
            return false;
        }
        int fd = m_socket ? m_socket->fd() : -1;
        return write(fd, buffer, stream.buffer_size()) == stream.buffer_size();
    }

private:
    void handle_messages();

    SharedPtr<App::UnixSocket> m_socket;
    SharedPtr<MessageDispatcher> m_dispatcher;
    Vector<UniquePtr<Message>> m_messages;
    String m_path;
};

}
