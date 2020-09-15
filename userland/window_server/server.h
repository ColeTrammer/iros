#pragma once

#include <app/object.h>
#include <app/selectable_file.h>
#include <app/timer.h>
#include <app/unix_socket.h>
#include <app/unix_socket_server.h>
#include <liim/pointers.h>
#include <liim/vector.h>
#include <window_server/message.h>

class Bitmap;
class WindowManager;

class Server final : public App::Object {
    APP_OBJECT(Server)

public:
    Server(int fb, SharedPtr<Bitmap> front_buffer, SharedPtr<Bitmap> back_buffer);
    ~Server();

    virtual void initialize() override;

    void start();

private:
    void kill_client(int client_id);

    void update_draw_timer();

    void handle_create_window_request(const WindowServer::Message& message, int client_id);
    void handle_remove_window_request(const WindowServer::Message& message, int client_id);
    void handle_change_window_visibility_request(const WindowServer::Message& message, int client_id);
    void handle_swap_buffer_request(const WindowServer::Message& message, int client_id);
    void handle_window_ready_to_resize_message(const WindowServer::Message& message, int client_id);
    void handle_window_rename_request(const WindowServer::Message& request, int client_id);

    UniquePtr<WindowManager> m_manager;
    SharedPtr<App::UnixSocketServer> m_socket_server;
    SharedPtr<App::SelectableFile> m_keyboard;
    SharedPtr<App::SelectableFile> m_mouse;
    SharedPtr<App::Timer> m_draw_timer;
    Vector<SharedPtr<App::UnixSocket>> m_clients;
};
