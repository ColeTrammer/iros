#include <app/widget.h>
#include <app/window.h>
#include <window_server/connection.h>

namespace App {

static WindowServer::Connection& connection() {
    static WindowServer::Connection* connection;
    if (!connection) {
        connection = new WindowServer::Connection;
    }
    return *connection;
}

Window::~Window() {}

Window::Window(int x, int y, int width, int height, String name) {
    m_ws_window = connection().create_window(x, y, width, height, move(name));
    m_ws_window->set_draw_callback([this](auto&) {
        for (auto& child : children()) {
            if (child->is_widget()) {
                static_cast<Widget&>(const_cast<Object&>(*child)).render();
            }
        }
    });
}

}
