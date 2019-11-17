#pragma once

#include <liim/vector.h>

class Server {
public:
    Server();
    ~Server();

    void start();

private:
    int m_socket_fd;
    Vector<int> m_clients;
};