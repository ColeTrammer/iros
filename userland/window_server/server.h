#pragma once

#include <liim/vector.h>
#include <memory>

class PixelBuffer;
class WindowManager;

class Server {
public:
    Server(std::shared_ptr<PixelBuffer> pixels);
    ~Server();

    void start();

private:
    std::shared_ptr<PixelBuffer> m_pixels;
    std::unique_ptr<WindowManager> m_manager;
    int m_socket_fd;
    Vector<int> m_clients;
};