#include <app/app.h>
#include <app/widget.h>
#include <app/window.h>
#include <window_server/connection.h>

namespace App {

Window::~Window() {}

Window::Window(int x, int y, int width, int height, String name) {
    m_ws_window = App::the().ws_connection().create_window(x, y, width, height, move(name));
    m_ws_window->set_draw_callback([this](auto&) {
        for (auto& child : children()) {
            if (child->is_widget()) {
                static_cast<Widget&>(const_cast<Object&>(*child)).render();
            }
        }
    });
}

}
