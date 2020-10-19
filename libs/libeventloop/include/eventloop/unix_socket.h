#pragma once

#include <eventloop/selectable.h>
#include <liim/string.h>

namespace App {

class UnixSocket final : public Selectable {
    APP_OBJECT(UnixSocket)

public:
    static SharedPtr<UnixSocket> create_from_fd(SharedPtr<Object> parent, int accepted_fd, bool nonblocking);
    static SharedPtr<UnixSocket> create_connection(SharedPtr<Object> parent, const String& path);

    ~UnixSocket();

    void disconnect();

    bool nonblocking() const { return m_nonblocking; }
    void set_nonblocking(bool b);

    Function<void(UnixSocket& self)> on_ready_to_read;
    Function<void(UnixSocket& self)> on_disconnect;

private:
    UnixSocket(int fd, bool nonblocking);

    virtual void notify_readable() override;

    bool m_nonblocking { true };
};
}
