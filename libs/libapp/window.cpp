#include <app/app.h>
#include <app/event.h>
#include <app/widget.h>
#include <app/window.h>
#include <liim/hash_map.h>
#include <window_server/connection.h>

namespace App {

static HashMap<wid_t, WeakPtr<Window>> s_windows;

void Window::register_window(const SharedPtr<Window>& window) {
    s_windows.put(window->wid(), window);
}

void Window::unregister_window(wid_t wid) {
    s_windows.remove(wid);
}

Maybe<WeakPtr<Window>> Window::find_by_wid(wid_t wid) {
    auto result = s_windows.get(wid);
    if (!result) {
        return {};
    }
    return { *result };
}

Window::~Window() {
    unregister_window(wid());
}

Window::Window(int x, int y, int width, int height, String name) {
    m_ws_window = App::the().ws_connection().create_window(x, y, width, height, move(name));
    m_ws_window->set_draw_callback([this](auto&) {
        for (auto& child : children()) {
            if (child->is_widget()) {
                static_cast<Widget&>(const_cast<Object&>(*child)).render();
            }
        }
    });
    set_rect({ 0, 0, width, height });
}

void Window::on_event(Event& event) {
    switch (event.type()) {
        case Event::Type::Window: {
            auto& window_event = static_cast<WindowEvent&>(event);
            if (window_event.window_event_type() == WindowEvent::Type::Close) {
                App::the().main_event_loop().set_should_exit(true);
                return;
            }
            break;
        }
        case Event::Type::Mouse: {
            auto& mouse_event = static_cast<MouseEvent&>(event);
            auto& widget = find_widget_at_point({ mouse_event.x(), mouse_event.y() });
            mouse_event.set_x(mouse_event.x() - widget.rect().x());
            mouse_event.set_y(mouse_event.y() - widget.rect().y());
            widget.on_mouse_event(mouse_event);
            break;
        }
        default:
            break;
    }
}

Widget& Window::find_widget_at_point(Point p) {
    Widget* parent = this;
    for (auto& child : parent->children()) {
        if (child->is_widget()) {
            auto& widget_child = const_cast<Widget&>(static_cast<const Widget&>(*child));
            if (widget_child.rect().intersects(p)) {
                parent = &widget_child;
            }
        }
    }
    return *parent;
}

}
