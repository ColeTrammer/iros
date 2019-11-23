#pragma once

#include <liim/vector.h>
#include <liim/pointers.h>

class PixelBuffer;
class WindowManager;

class Server {
public:
    Server(SharedPtr<PixelBuffer> pixels);
    ~Server();

    void start();

private:
    SharedPtr<PixelBuffer> m_pixels;
    UniquePtr<WindowManager> m_manager;
    int m_socket_fd;
    Vector<int> m_clients;
};