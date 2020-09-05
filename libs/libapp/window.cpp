#include <app/app.h>
#include <app/context_menu.h>
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

Window::Window(int x, int y, int width, int height, String name, WindowServer::WindowType type, wid_t parent_id) {
    m_ws_window = App::the().ws_connection().create_window(x, y, width, height, move(name), type, parent_id);
    m_ws_window->set_draw_callback([this](auto&) {
        if (m_main_widget && !m_main_widget->hidden()) {
            m_main_widget->render();
        }
    });
    set_rect({ 0, 0, width, height });
    register_window(*this);
}

void Window::set_rect(const Rect& rect) {
    m_rect = rect;
    if (m_main_widget) {
        m_main_widget->set_rect(rect);
    }
}

void Window::hide() {
    if (!m_visible) {
        return;
    }

    m_visible = false;
    m_ws_window->set_visibility(0, 0, false);
}

void Window::show(int x, int y) {
    if (m_visible) {
        return;
    }

    m_visible = true;
    m_ws_window->set_visibility(x, y, true);
}

void Window::hide_current_context_menu() {
    auto maybe_context_menu = m_current_context_menu.lock();
    if (maybe_context_menu) {
        maybe_context_menu->hide();
    }
}

void Window::on_event(Event& event) {
    switch (event.type()) {
        case Event::Type::Window: {
            auto& window_event = static_cast<WindowEvent&>(event);
            if (window_event.window_event_type() == WindowEvent::Type::Close) {
                m_ws_window->set_removed(true);
                App::the().main_event_loop().set_should_exit(true);
                return;
            }
            if (window_event.window_event_type() == WindowEvent::Type::DidResize) {
                auto response = App::the().ws_connection().send_window_ready_to_resize_message(wid());
                assert(response->type == WindowServer::Message::Type::WindowReadyToResizeResponse);
                auto& data = response->data.window_ready_to_resize_response;
                assert(data.wid == wid());

                if (data.new_width == m_ws_window->rect().width() && data.new_height == m_ws_window->rect().height()) {
                    return;
                }

                m_ws_window->resize(data.new_width, data.new_height);
                pixels()->clear();
                set_rect({ 0, 0, data.new_width, data.new_height });
                invalidate_rect(rect());
                return;
            }
            break;
        }
        case Event::Type::WindowState: {
            auto& state_event = static_cast<WindowStateEvent&>(event);
            if (state_event.active() == active()) {
                return;
            }

            if (!state_event.active()) {
                did_become_inactive();
            } else {
                did_become_active();
            }
            m_active = state_event.active();
            return;
        }
        case Event::Type::Mouse: {
            auto& mouse_event = static_cast<MouseEvent&>(event);
            Widget* widget = nullptr;
            if (mouse_event.left() == MOUSE_DOWN) {
                m_left_down = true;
            } else if (mouse_event.left() == MOUSE_UP) {
                m_left_down = false;
            }

            if (mouse_event.right() == MOUSE_DOWN) {
                m_right_down = true;
            } else if (mouse_event.right() == MOUSE_UP) {
                m_right_down = false;
            }

            hide_current_context_menu();
            if (mouse_event.left() == MOUSE_NO_CHANGE && mouse_event.right() == MOUSE_NO_CHANGE && m_right_down && m_left_down) {
                if (!focused_widget()) {
                    return;
                }
                widget = focused_widget().get();
            } else {
                widget = find_widget_at_point({ mouse_event.x(), mouse_event.y() });
                if (focused_widget() && focused_widget().get() != widget) {
                    focused_widget()->on_leave();
                }
                set_focused_widget(widget);
            }

            if (widget) {
                mouse_event.set_x(mouse_event.x() - widget->rect().x());
                mouse_event.set_y(mouse_event.y() - widget->rect().y());
                widget->on_mouse_event(mouse_event);
            }
            return;
        }
        case Event::Type::Key: {
            auto& key_event = static_cast<KeyEvent&>(event);
            auto widget = focused_widget();
            if (widget) {
                widget->on_key_event(key_event);
            }
            return;
        }
        default:
            break;
    }

    Object::on_event(event);
}

Widget* Window::find_widget_at_point(Point p) {
    Widget* parent = m_main_widget.get();
    while (parent && !parent->children().empty()) {
        bool found = false;
        for (auto& child : parent->children()) {
            if (child->is_widget()) {
                auto& widget_child = const_cast<Widget&>(static_cast<const Widget&>(*child));
                if (!widget_child.hidden() && widget_child.rect().intersects(p)) {
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
    return parent;
}

void Window::set_focused_widget(Widget* widget) {
    if (!widget) {
        m_focused_widget.reset();
        return;
    }

    m_focused_widget = widget->weak_from_this();
    widget->on_focused();
}

SharedPtr<Widget> Window::focused_widget() {
    auto ret = m_focused_widget.lock();
    if (!ret) {
        set_focused_widget(nullptr);
    }
    return ret;
}

void Window::clear_current_context_menu() {
    m_current_context_menu.reset();
}

void Window::set_current_context_menu(ContextMenu* menu) {
    m_current_context_menu = menu->weak_from_this();
}

void Window::draw() {
    m_ws_window->draw();
}

void Window::invalidate_rect(const Rect& rect) {
    if (rect.width() == 0 || rect.height() == 0) {
        return;
    }

    if (m_will_draw_soon) {
        return;
    }

    m_will_draw_soon = true;
    deferred_invoke([this] {
        draw();
        m_will_draw_soon = false;
    });
}

}
