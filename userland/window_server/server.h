#pragma once

#include <eventloop/object.h>
#include <eventloop/selectable_file.h>
#include <eventloop/timer.h>
#include <eventloop/unix_socket.h>
#include <eventloop/unix_socket_server.h>
#include <ipc/server.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <window_server/message.h>

class Bitmap;
class WindowManager;

namespace WindowServer {
class ServerImpl final : public Client::MessageDispatcher {
    APP_OBJECT(ServerImpl)

public:
    ServerImpl(int fb, SharedPtr<Bitmap> front_buffer, SharedPtr<Bitmap> back_buffer);
    ~ServerImpl();

    virtual void initialize() override;

    void start();

private:
    void update_draw_timer();
    void kill_client(IPC::Endpoint& client);

    virtual void handle_error(IPC::Endpoint& client) override { kill_client(client); }

    virtual void handle(IPC::Endpoint& client, const Client::CreateWindowRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::RemoveWindowRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::ChangeWindowVisibilityRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::SwapBufferRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::WindowReadyToResizeMessage& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::WindowRenameRequest& data) override;

    UniquePtr<WindowManager> m_manager;
    SharedPtr<IPC::Server> m_server;
    SharedPtr<App::FdWrapper> m_input_socket;
    SharedPtr<App::Timer> m_draw_timer;
};
}
