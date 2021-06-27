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
class Window;

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

    void notify_listeners_did_create_window(const Window& window);
    void notify_listeners_did_change_window_title(const Window& window);
    void notify_listeners_did_close_window(wid_t wid);
    void notify_listeners_did_make_window_active(const Window& window);

    virtual void handle_error(IPC::Endpoint& client) override { kill_client(client); }

    virtual void handle(IPC::Endpoint& client, const Client::CreateWindowRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::RemoveWindowRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::ChangeWindowVisibilityRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::SwapBufferRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::WindowReadyToResizeMessage& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::WindowRenameRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::ChangeThemeRequest& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::RegisterAsWindowServerListener& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::UnregisterAsWindowServerListener& data) override;
    virtual void handle(IPC::Endpoint& client, const Client::SetActiveWindow& data) override;

    UniquePtr<WindowManager> m_manager;
    SharedPtr<IPC::Server> m_server;
    SharedPtr<App::FdWrapper> m_input_socket;
    SharedPtr<App::Timer> m_draw_timer;
    HashSet<IPC::Endpoint*> m_window_server_listeners;
};
}
