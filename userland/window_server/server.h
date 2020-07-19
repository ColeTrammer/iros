#pragma once

#include <liim/pointers.h>
#include <liim/vector.h>
#include <window_server/message.h>

class PixelBuffer;
class WindowManager;

class Server {
public:
    Server(int fb, SharedPtr<PixelBuffer> front_buffer, SharedPtr<PixelBuffer> back_buffer);
    ~Server();

    void start();

private:
    void kill_client(int client_id);

    void handle_create_window_request(const WindowServer::Message& message, int client_id);
    void handle_remove_window_request(const WindowServer::Message& message, int client_id);
    void handle_swap_buffer_request(const WindowServer::Message& message, int client_id);
    void handle_window_ready_to_resize_message(const WindowServer::Message& message, int client_id);
    void handle_window_rename_request(const WindowServer::Message& request, int client_id);

    UniquePtr<WindowManager> m_manager;
    int m_socket_fd;
    int m_kbd_fd;
    int m_mouse_fd;
    Vector<int> m_clients;
};
