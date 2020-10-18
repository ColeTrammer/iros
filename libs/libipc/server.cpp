#include <ipc/message_dispatcher.h>
#include <ipc/server.h>
#include <sys/stat.h>
#include <unistd.h>

namespace IPC {

Server::Server(String path, SharedPtr<MessageDispatcher> dispatcher) : m_path(move(path)), m_dispatcher(move(dispatcher)) {}

void Server::initialize() {
    m_socket = App::UnixSocketServer::create(shared_from_this(), m_path);
    m_socket->on_ready_to_accept = [this] {
        SharedPtr<App::UnixSocket> client_socket;
        while (client_socket = m_socket->accept()) {
            auto client = Endpoint::create(shared_from_this());
            client->set_socket(move(client_socket));
            client->set_dispatcher(m_dispatcher);
            m_clients.add(move(client));
        }
    };
}

Server::~Server() {}

void Server::kill_client(Endpoint& client) {
    auto ptr = client.shared_from_this();
    m_clients.remove_element(ptr);
    remove_child(ptr);
}

}
