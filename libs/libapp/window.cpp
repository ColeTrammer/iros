#include <app/app.h>
#include <app/event.h>
#include <app/widget.h>
#include <app/window.h>
#include <liim/hash_map.h>
#include <window_server/connection.h>

namespace App {

static HashMap<wid_t, Window*> s_windows;

void Window::register_window(Window& window) {
    s_windows.put(window.wid(), &window);
}

void Window::unregister_window(wid_t wid) {
    s_windows.remove(wid);
}

Maybe<SharedPtr<Window>> Window::find_by_wid(wid_t wid) {
    auto result = s_windows.get(wid);
    if (!result) {
        return {};
    }
    return { (*result)->shared_from_this() };
}

Window::~Window() {
    unregister_window(wid());
}

Window::Window(int x, int y, int width, int height, String name) {
    m_ws_window = App::the().ws_connection().create_window(x, y, width, height, move(name));
    m_ws_window->set_draw_callback([this](auto&) {
        render();
    });
    set_rect({ 0, 0, width, height });
    register_window(*this);
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
            set_focused_widget(widget);
            widget.on_mouse_event(mouse_event);
            break;
        }
        case Event::Type::Key: {
            auto& key_event = static_cast<KeyEvent&>(event);
            auto widget = focused_widget();
            widget->on_key_event(key_event);
            break;
        }
        case Event::Type::Resize: {
            auto& resize_event = static_cast<ResizeEvent&>(event);
            m_ws_window->resize(resize_event.new_width(), resize_event.new_height());
            set_rect({ rect().x(), rect().y(), resize_event.new_width(), resize_event.new_height() });
            pixels()->clear();
            draw();
            break;
        }
        default:
            break;
    }
}

Widget& Window::find_widget_at_point(Point p) {
    Widget* parent = this;
    while (!parent->children().empty()) {
        bool found = false;
        for (auto& child : parent->children()) {
            if (child->is_widget()) {
                auto& widget_child = const_cast<Widget&>(static_cast<const Widget&>(*child));
                if (widget_child.rect().intersects(p)) {
                    parent = &widget_child;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            break;
        }
    }
    return *parent;
}

void Window::set_focused_widget(Widget& widget) {
    m_focused_widget = move(widget.weak_from_this());
}

SharedPtr<Widget> Window::focused_widget() {
    auto ret = m_focused_widget.lock();
    if (!ret) {
        set_focused_widget(*this);
        return shared_from_this();
    }
    return move(ret);
}

}
