#pragma once

#include <app/selectable.h>
#include <liim/string.h>

namespace App {

class UnixSocket final : public Selectable {
    APP_OBJECT(UnixSocket)

public:
    static SharedPtr<UnixSocket> create_from_fd(SharedPtr<Object> parent, int accepted_fd);
    static SharedPtr<UnixSocket> create_connection(SharedPtr<Object> parent, const String& path);

    ~UnixSocket();

    void disconnect();

    Function<void(UnixSocket& self)> on_ready_to_read;
    Function<void(UnixSocket& self)> on_disconnect;

private:
    UnixSocket(int fd);

    virtual void notify_readable() override;
};

}
