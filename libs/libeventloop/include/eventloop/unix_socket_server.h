#pragma once

#include <eventloop/selectable.h>
#include <eventloop/unix_socket.h>
#include <liim/function.h>
#include <liim/string.h>

namespace App {

class UnixSocketServer final : public Selectable {
    APP_OBJECT(UnixSocketServer)

public:
    explicit UnixSocketServer(const String& bind_path);
    virtual ~UnixSocketServer();

    SharedPtr<UnixSocket> accept();
};

}
