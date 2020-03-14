#pragma once

#include <liim/pointers.h>
#include <window_server/connection.h>
#include <window_server/window.h>

class WindowWrapper {
public:
    WindowWrapper();
    ~WindowWrapper();

    int width() const { return window().rect().width() / 8; }
    int height() const { return window().rect().height() / 16; }

    size_t size() const { return window().rect().width() * window().rect().height(); }
    size_t size_in_bytes() const { return window().rect().width() * window().rect().height() * sizeof(uint32_t); }

    WindowServer::Window& window() { return *m_window; }
    const WindowServer::Window& window() const { return *m_window; }

private:
    WindowServer::Connection m_connection;
    SharedPtr<WindowServer::Window> m_window;
};