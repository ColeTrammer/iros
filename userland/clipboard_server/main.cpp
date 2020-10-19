#include <assert.h>
#include <clipboard_server/message.h>
#include <eventloop/event_loop.h>
#include <eventloop/unix_socket.h>
#include <eventloop/unix_socket_server.h>
#include <ipc/server.h>
#include <liim/string.h>
#include <liim/vector.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

namespace ClipboardServer {
class Dispatcher final : public Client::MessageDispatcher {
    APP_OBJECT(Dispatcher)

public:
    virtual void initialize() override {
        m_server = IPC::Server::create(shared_from_this(), "/tmp/.clipboard_server.socket", shared_from_this());
    }

    virtual void handle_error(IPC::Endpoint& client) override { m_server->kill_client(client); }

    virtual void handle(IPC::Endpoint& client, const Client::GetContentsRequest& message) override {
        Server::GetContentsResponse response;
        if (m_data_type == message.type) {
            response.data = m_data;
        }
        send(client, response);
    }

    virtual void handle(IPC::Endpoint& client, const Client::SetContentsRequest& message) override {
        m_data = message.data;
        m_data_type = message.type;

        syslog(LOG_INFO, "set clipboard content: type='%s' data='%s'", m_data_type.string(),
               String(StringView(m_data.vector(), m_data.vector() + m_data.size() - 1)).string());

        send<Server::SetContentsResponse>(client, { .success = true });
    }

private:
    SharedPtr<IPC::Server> m_server;
    Vector<char> m_data;
    String m_data_type;
};
}

int main() {
    using namespace ClipboardServer;

    auto dispatcher = Dispatcher::create(nullptr);

    App::EventLoop loop;
    loop.enter();
    return 0;
}
