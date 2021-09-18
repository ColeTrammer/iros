#pragma once

#include <eventloop/event.h>
#include <eventloop/selectable.h>
#include <liim/string.h>

APP_EVENT(App, DisconnectEvent, Event, (), (), ())

namespace App {
class UnixSocket final : public Selectable {
    APP_OBJECT(UnixSocket)

    APP_EMITS(Selectable, DisconnectEvent)

public:
    static SharedPtr<UnixSocket> create_from_fd(SharedPtr<Object> parent, int accepted_fd, bool nonblocking);
    static SharedPtr<UnixSocket> create_connection(SharedPtr<Object> parent, const String& path);

    ~UnixSocket();

    void disconnect();

    bool nonblocking() const { return m_nonblocking; }
    void set_nonblocking(bool b);

private:
    UnixSocket(int fd, bool nonblocking);

    bool m_nonblocking { true };
};
}
