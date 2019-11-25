#pragma once

#include <liim/vector.h>
#include <memory>
#include <window_server/message.h>

class PixelBuffer;
class WindowManager;

class Server {
public:
    Server(std::shared_ptr<PixelBuffer> pixels);
    ~Server();

    void start();

private:
    void kill_client(int client_id);

    void handle_create_window_request(const WindowServer::Message&, int client_id);
    void handle_remove_window_request(const WindowServer::Message&, int client_id);

    std::unique_ptr<WindowManager> m_manager;
    int m_socket_fd;
    Vector<int> m_clients;
};