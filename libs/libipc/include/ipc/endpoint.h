#pragma once

#include <app/unix_socket.h>
#include <ipc/message.h>
#include <ipc/message_dispatcher.h>

namespace IPC {

class Endpoint : public App::Object {
public:
    Endpoint(String path) : m_path(move(path)) {}
    virtual void initialize() override;

    void set_dispatcher(SharedPtr<MessageDispatcher> dispatcher) { m_dispatcher = move(dispatcher); }

private:
    void handle_messages();

    SharedPtr<App::UnixSocket> m_socket;
    SharedPtr<MessageDispatcher> m_dispatcher;
    Vector<UniquePtr<Message>> m_messages;
    String m_path;
};

}
