#include <app/app.h>
#include <app/context_menu.h>
#include <app/widget.h>
#include <app/window.h>
#include <eventloop/event.h>

namespace App {

static HashMap<wid_t, Window*> s_windows;

const HashMap<wid_t, Window*>& Window::windows() {
    return s_windows;
}

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

    if (m_raw_pixels != MAP_FAILED) {
        munmap(m_raw_pixels, m_raw_pixels_size);
        m_raw_pixels = MAP_FAILED;
    }

    if (!m_removed) {
        App::the().ws().server().send<WindowServer::Client::RemoveWindowRequest>({ .wid = m_wid });
        m_removed = true;
    }
}

Window::Window(int x, int y, int width, int height, String name, bool has_alpha, WindowServer::WindowType type, wid_t parent_id)
    : m_has_alpha(has_alpha) {
    auto response =
        App::the().ws().server().send_then_wait<WindowServer::Client::CreateWindowRequest, WindowServer::Server::CreateWindowResponse>({
            .x = x,
            .y = y,
            .width = width,
            .height = height,
            .name = move(name),
            .type = type,
            .parent_id = parent_id,
            .has_alpha = has_alpha,
        });
    assert(response.has_value());

    m_shm_path = response.value().path;
    m_wid = response.value().wid;

    do_resize(width, height);
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
    do_set_visibility(0, 0, false);
}

void Window::show(int x, int y) {
    if (m_visible) {
        return;
    }

    m_visible = true;
    do_set_visibility(x, y, true);
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
                m_removed = true;
                App::the().main_event_loop().set_should_exit(true);
                return;
            }
            if (window_event.window_event_type() == WindowEvent::Type::DidResize) {
                auto response = App::the()
                                    .ws()
                                    .server()
                                    .send_then_wait<WindowServer::Client::WindowReadyToResizeMessage,
                                                    WindowServer::Server::WindowReadyToResizeResponse>({ .wid = m_wid });
                assert(response.has_value());
                auto& data = response.value();
                assert(data.wid == wid());

                if (data.new_width == rect().width() && data.new_height == rect().height()) {
                    return;
                }

                do_resize(data.new_width, data.new_height);
                if (auto* main_widget = m_main_widget.get()) {
                    main_widget->set_rect({ 0, 0, data.new_width, data.new_height });
                }
                pixels()->clear(App::the().palette()->color(Palette::Background));
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
        case Event::Type::ThemeChange:
            m_back_buffer->clear(App::the().palette()->color(Palette::Background));
            invalidate_rect(rect());

            m_main_widget->on_theme_change_event(static_cast<ThemeChangeEvent&>(event));
            return;
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

void Window::do_set_visibility(int x, int y, bool visible) {
    App::the()
        .ws()
        .server()
        .send_then_wait<WindowServer::Client::ChangeWindowVisibilityRequest, WindowServer::Server::ChangeWindowVisibilityResponse>({
            .wid = m_wid,
            .x = x,
            .y = y,
            .visible = visible,
        });
}

void Window::do_resize(int new_width, int new_height) {
    if (m_raw_pixels != MAP_FAILED) {
        munmap(m_raw_pixels, m_raw_pixels_size);
        m_raw_pixels = MAP_FAILED;
    }

    int shm = shm_open(m_shm_path.string(), O_RDWR, 0);
    assert(shm != -1);

    m_raw_pixels_size = 2 * new_width * new_height * sizeof(uint32_t);
    m_raw_pixels = mmap(nullptr, m_raw_pixels_size, PROT_WRITE | PROT_READ, MAP_SHARED, shm, 0);
    assert(m_raw_pixels != MAP_FAILED);
    close(shm);

    m_front_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels), new_width, new_height, has_alpha());
    m_back_buffer = Bitmap::wrap(reinterpret_cast<uint32_t*>(m_raw_pixels) + m_raw_pixels_size / 2 / sizeof(uint32_t), new_width,
                                 new_height, has_alpha());

    m_rect.set_width(new_width);
    m_rect.set_height(new_height);
}

void Window::draw() {
    if (m_main_widget && !m_main_widget->hidden()) {
        m_main_widget->render();

        App::the().ws().server().send<WindowServer::Client::SwapBufferRequest>({ .wid = m_wid });
        LIIM::swap(m_front_buffer, m_back_buffer);
        memcpy(m_back_buffer->pixels(), m_front_buffer->pixels(), m_front_buffer->size_in_bytes());
    }
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
