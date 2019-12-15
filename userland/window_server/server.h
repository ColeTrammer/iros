#pragma once

#include <liim/vector.h>
#include <memory>
#include <window_server/message.h>

class PixelBuffer;
class WindowManager;

class Server {
public:
    Server(int fb, std::shared_ptr<PixelBuffer> front_buffer, std::shared_ptr<PixelBuffer> back_buffer);
    ~Server();

    void start();

private:
    void kill_client(int client_id);

    void handle_create_window_request(const WindowServer::Message&, int client_id);
    void handle_remove_window_request(const WindowServer::Message&, int client_id);
    void handle_swap_buffer_request(const WindowServer::Message&, int client_id);

    std::unique_ptr<WindowManager> m_manager;
    int m_socket_fd;
    Vector<int> m_clients;
};