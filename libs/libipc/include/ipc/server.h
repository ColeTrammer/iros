#pragma once

#include <eventloop/unix_socket_server.h>
#include <ipc/endpoint.h>

namespace IPC {

class Server : public App::Object {
    APP_OBJECT(Server)

public:
    Server(String path);
    virtual void initialize() override;

    void kill_client(Endpoint& client);

private:
    String m_path;
    SharedPtr<App::UnixSocketServer> m_socket;
    Vector<SharedPtr<Endpoint>> m_clients;
};

}
